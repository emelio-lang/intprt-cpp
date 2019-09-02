/* ========================================================================
   $File: notation.cpp $
   $Date: Sep 09 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"
#include "notation.h"

inline bool is_notation_variable(const string& s) {
    bool res = true;
    for (char c : s) if (!isupper(c)) res = false;
    return res;
}

inline bool is_notation_free_variable(const string& s) {
    bool res = true;
    for (int i = 0; i < s.size()-1; ++i)
        if (!isupper(s[i])) res = false;
    if (s.back() != 's') res = false;
    return res;
}

// NOTE: 左右に変数がない場合、適当な文字を入れ込みます
void pad_notation_config(vector<string>& config) {
    if (!is_notation_variable(config[0])) {
        config.insert(config.begin(), random_saneupper_string(16));
    }
    if (!is_notation_variable(config.back())) {
        config.push_back(random_saneupper_string(16));
    }
}

void replace_code(shared_ptr<Code> c, const map<string, shared_ptr<Code>> &d) {
    if (c->l) {
        for (string &a : c->l->argnames) {
            if (CONTAINS(d, a)) {
                a = d.at(a)->lit.val; // ?
            }
        }

        replace_code(c->l->body, d);
    } else if (c->lit.val != "") {
        if (CONTAINS(d, c->lit.val) &&
            (is_notation_variable(c->lit.val) || is_notation_free_variable(c->lit.val))
            )
        {
            if (c->args.size() == 0) {
                *c = *d.at(c->lit.val);
                return;
            } else {
                if (!c->l)
                    c->l = shared_ptr<Lambda>(new Lambda);
            
                c->l->body = make_shared<Code>(*d.at(c->lit.val));
                // else
                //     c->l = shared_ptr<Lambda>(nullptr);
                // c->lit = d.at(c->lit.val)->lit;
            }
        }
    }

    
//    for (auto &a : c->args) {
    for (int i = 0; i < c->args.size(); ++i) {
        replace_code(c->args[i], d); // ?;
    }
}

// TODO: notation適用後のsrcがおかしいせいでバグってる
// TODO: 上の問題の解決のため、まずは適用する時に無駄な関数が一枚噛まされているのを直したい
bool apply_notation_greedily(shared_ptr<Code> &code, const Notation& notation) {
    // first, check length
    if (code->args.size()+1 < notation.config.size())
        return false;


    map<string, shared_ptr<Code>> d;
    // if (!is_notation_variable(*notation.config.beg) && code->lit.val != *notation.config.beg) return;
    // d.insert(make_pair(*notation.config.beg, code));

    int i = 0;
    for (auto i_notval = notation.config.beg;
         i_notval != notation.config.end;
         i_notval++)
    {
        // NOTE: 大きさのチェックは最初にしたのでこのときのみがチェック対象
        if (i >= code->args.size()) break;

        // free variable
        // 文字列を集めてパースただし、関数が潜んでいた場合... = add (f 3) 2;とか
        if (is_notation_free_variable(*i_notval)) {
            int start = i;
            shared_ptr<Code> tmp;

            // NOTE: tmpにはいまからargsを入れていくので、空でないと駄目
            // 既存のargsからとってくる時は、argsとしてあるcodeにargsはないので面倒な処理をパスしている
            // おかしかったら下の方の[ERROR]が発動する
            if (i_notval == notation.config.beg) {
                tmp = shared_ptr<Code>(new Code);
                tmp->l = code->l;
                tmp->lit = code->lit;
                // codeの引数を抜かした部分のsrcが欲しい
                if (code->l) tmp->src = code->l->body->src;
                else tmp->src = TknvalsRegion {code->src.beg, next(code->src.beg)};
                
            } else {
                tmp = make_shared<Code>(*code->args[i]);
                i++;
            }

            if (tmp->args.size() != 0) {
                cout << "[ERROR] 何かがおかしい" << endl;
            }

            while (true) {
                if (i >= code->args.size()
                    || 
                    code->args[i]->lit.val == *next(i_notval))
                {
                    d.insert(make_pair(*i_notval, tmp));
                    break;
                }

                tmp->args.push_back(code->args[i]);
                tmp->src.end = code->args[i]->src.end;
                
                i++;
            }
        } else {
            if (i_notval == notation.config.beg) {
                if (!is_notation_variable(*i_notval) && code->lit.val != *i_notval) return false;
                d.insert(make_pair(*i_notval, code));
            } else {
                if (!is_notation_variable(*i_notval) && code->args[i]->lit.val != *i_notval) return false;
                d.insert(make_pair(*i_notval, code->args[i]));
                i++;
            }
        }
    }

    // 十分にマッチできていない
    if (d.size() != notation.config.size()) return false;

// Code fruit;
    // fruit.deep_copy_from(notation.to);
    code = shared_ptr<Code> (new Code);
    code->deep_copy_from(*notation.to);

    cout << "before" << endl;
    cout << *code << endl << endl;

    for (auto it = d.begin(); it != d.end(); it++) {
        cout << "{" << it->first << "}" << endl;
        cout << *it->second << endl << endl;
    }

    replace_code(code, d);

    cout<< "after" << endl;
    cout << *code << endl << endl;

    return true;
}

bool check_match_code(TknvalsRegion config, const shared_ptr<Code> &code, int index, bool ignore_first_free_variable) {
    if (ignore_first_free_variable && is_notation_free_variable(*config.beg)) {
        config.beg++;
    }

    int res_count = 0;
    int i = index;
    for (auto i_notval = config.beg;
         i_notval != config.end;
         i_notval++)
    {
        // NOTE: 大きさのチェックは最初にしたのでこのときのみがチェック対象
        if (i >= code->args.size()) break;

        // free variable
        // 文字列を集めてパースただし、関数が潜んでいた場合... = add (f 3) 2;とか
        if (is_notation_free_variable(*i_notval)) {
            int start = i;
            if (i_notval != config.beg) {
                i++;
            }

            while (true) {
                if (i >= code->args.size()
                    || 
                    code->args[i]->lit.val == *next(i_notval)
                    )
                {
                    res_count++;
                    break;
                }
                
                i++;
            }
        } else {
            if (i_notval == config.beg && !ignore_first_free_variable) {
                if (!is_notation_variable(*i_notval) && code->lit.val != *i_notval) return false;
                res_count++;
            } else {
                if (!is_notation_variable(*i_notval) && code->args[i]->lit.val != *i_notval) return false;
                res_count++;
                i++;
            }
        }
    }

    // 十分にマッチできていない
    return res_count == config.size();
}


bool
match_code(
    TknvalsRegion config,
    const shared_ptr<Code> &code,
    map<string, shared_ptr<Code>> *res,
    vector<shared_ptr<Code>> *remains = NULL
           )
{
    string exit_token = "";
    {
        auto i_notval = config.beg;
        while (is_notation_variable(*i_notval) || is_notation_free_variable(*i_notval)) {
            i_notval++;
            if (i_notval == config.end) {
                cout << "[ERROR] notationのexit_tokenが見つかりませんでした" << endl;
                break;
            }
        }
        exit_token = *i_notval;
    }

    int res_count = 0;
    int i = 0;
    bool perdu = false;
    bool remain = false;
    for (auto i_notval = config.beg;
         i_notval != config.end;
         i_notval++)
    {
        // NOTE: 大きさのチェックは最初にしたのでこのときのみがチェック対象
        if (i >= code->args.size()) break;
        if (remain) {
            remains->push_back(code->args[i]);
            i_notval--; // NOTE: remain処理のためforループ脱出防止
            i++;
            continue;
        }

        // free variable
        // 文字列を集めてパースただし、関数が潜んでいた場合... = add (f 3) 2;とか
        if (is_notation_free_variable(*i_notval)) {
            int start = i;
            shared_ptr<Code> tmp;

            // NOTE: tmpにはいまからargsを入れていくので、空でないと駄目
            // 既存のargsからとってくる時は、argsとしてあるcodeにargsはないので面倒な処理をパスしている
            // おかしかったら下の方の[ERROR]が発動する
            if (i_notval == config.beg) {
                tmp = shared_ptr<Code>(new Code);
                tmp->l = code->l;
                tmp->lit = code->lit;
                // codeの引数を抜かした部分のsrcが欲しい
                if (code->l) tmp->src = code->l->body->src;
                else tmp->src = TknvalsRegion {code->src.beg, next(code->src.beg)};
                
            } else {
                tmp = make_shared<Code>(*code->args[i]);
                i++;
            }

            if (tmp->args.size() != 0) {
                cout << "[ERROR] 何かがおかしい" << endl;
            }

            while (true) {
                if (i >= code->args.size()
                    || 
                    code->args[i]->lit.val == *next(i_notval)
                    )
                {
                    if (res) res->insert(make_pair(*i_notval, tmp));
                    res_count++;
                    break;
                }

                if (perdu && code->args[i]->lit.val == exit_token && prev(config.end) == i_notval) {
                    // もしかしたら同じnotationがもう一回繰り返しているかもしれない --> チェックする
                    if (check_match_code(config, code, i, true)) {
                        // 繰り返しているならその直前で辞める
                        // remainがあるなら残りをremainsに格納して終了
                        if (remains != NULL) {
                            remain = true;
                        }

                        if (res) res->insert(make_pair(*i_notval, tmp));
                        res_count++;
                        i_notval--; // NOTE: remain処理のためforループ脱出防止
                        break;
                    }
                    // 思い違いなら続けて
                }

                tmp->args.push_back(code->args[i]);
                tmp->src.end = code->args[i]->src.end;
                
                i++;
            }
        } else {
            if (*i_notval == exit_token) perdu = true;
            if (i_notval == config.beg) {
                if (!is_notation_variable(*i_notval) && code->lit.val != *i_notval) return false;
                if (res) res->insert(make_pair(*i_notval, code));
                res_count++;
            } else {
                if (!is_notation_variable(*i_notval) && code->args[i]->lit.val != *i_notval) return false;
                if (res) res->insert(make_pair(*i_notval, code->args[i]));
                res_count++;
                i++;
            }
        }
    }

    // 十分にマッチできていない
    return res_count/*res.size()*/ == config.size();
}

bool apply_notation(shared_ptr<Code> &code, const Notation& notation) {
    // first, check length
    if (code->args.size()+1 < notation.config.size())
        return false;


    map<string, shared_ptr<Code>> d;
    vector<shared_ptr<Code>> remains;
    
    // if (!is_notation_variable(*notation.config.beg) && code->lit.val != *notation.config.beg) return false;
    // d.insert(make_pair(*notation.config.beg, code));
    if (!check_match_code(notation.config, code, 0, false)) return false;
    if (!match_code(notation.config, code, &d, &remains)) return false;


// Code fruit;
    // fruit.deep_copy_from(notation.to);
    code = shared_ptr<Code> (new Code);
    code->deep_copy_from(*notation.to);

    cout << "before" << endl;
    cout << *code << endl << endl;

    for (auto it = d.begin(); it != d.end(); it++) {
        cout << "{" << it->first << "}" << endl;
        cout << *it->second << endl << endl;
    }

    replace_code(code, d);
    if (remains.size() != 0) {
        shared_ptr<Code> tmp = code;
        code = shared_ptr<Code> (new Code);
        code->l = shared_ptr<Lambda> (new Lambda);
        code->l->body = tmp;
        code->args = remains;
        
        cout<< "after(remain)" << endl;
        cout << *code << endl << endl;
        
        apply_notation(code, notation);
    }

    cout<< "after" << endl;
    cout << *code << endl << endl;

    return true;
}
