/* ========================================================================
   $File: reduction.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"

{! BUILTIN_1 <> name <> Lambda #name = { { "a1" }, {} }; Code c_#name = { &::#name, "#name", {} }; !}
{! BUILTIN_2 <> name <> Lambda #name = { { "a1", "a2" }, {} }; Code c_#name = { &::#name, "#name", {} }; !}

{- BUILTIN_1 <> negate -}
{- BUILTIN_2 <> add -}

struct ReductionFlow {
    map<string, Code*> bind = {
        { "add", &c_add },
        { "negate", &c_negate },
    };
    stack<Code*> argstack;
    // NOTE: 未計算のままshadowingするために必要
    map<string, string> shadower;
};

struct TknvalsRegion {
    vector<string>::iterator beg, end;
};


// trim from end (in place)
template<class T>
static inline void crtrim(std::vector<T> &v, T target) {
    v.erase(std::find_if(v.rbegin(), v.rend(), [&](T ch) {
                                                   return ch != target;
                                               }).base(), v.end());
}

// trim from end (in place)
template<class T>
static inline void cltrim(std::vector<T> &v, T target) {
    v.erase(v.begin(), std::find_if(v.begin(), v.end(), [&](T ch) {
        return ch != target;
    }));
}

// trim from end (in place)
static inline void rmvparen(std::vector<string> &v) {
    if (v.size() == 0) return;

    if (v.front() == "(") v.erase(v.begin());
    if (v.back() == ")") v.pop_back();
}


bool check_computed(const Code* c) {
    return (c && !c->l);
}

pair<int,bool> read_int_litcode(const Code* c) {
    if (!check_computed(c)) {
        if (c) {
            cout << "[ERROR] (TODO:カインド) 次のようなCodeをIntリテラルとして読もうとしました" << endl;
            cout << *c << endl;
        }
        return make_pair(0, false);
    } 

    return make_pair(stoi(c->lit.val), true);
}

pair<int,bool> read_int_litcode(const Code& c) {
    if (!check_computed(&c)) {
        cout << "[ERROR] (TODO:カインド) 次のようなCodeをIntリテラルとして読もうとしました" << endl;
        cout << c << endl;
        return make_pair(0, false);
    } 

    return make_pair(stoi(c.lit.val), true);
}

Code make_int_litcode(int n) {
    return Code { 0, Literal { to_string(n) }, {} };
}

Code *ref(string name, ReductionFlow &rf) {
    if (CONTAINS(rf.shadower, name)) {
        string probe = rf.bind[rf.shadower[name]]->lit.val;
        if (CONTAINS(rf.shadower, probe)) {
            return rf.bind[probe]; 
        }
        // while (CONTAINS(rf.shadower, probe)) {
        //     name = probe;
        //     probe = rf.bind[probe]->lit.val;
        // }
        
        return rf.bind[rf.shadower[name]];
    }
    return rf.bind[name];
}
#define ref(a) ref(a,rf)

void forward_replace(vector<string>& pre_fruit, const vector<string>& config, const vector<string>& to) {
    while (true) {
        map<string, TknvalsRegion> matches;
        int c = 0;
        bool done = true;

        for (auto itr = pre_fruit.begin();
             itr != pre_fruit.end();
             ++itr)
        {
            if (itr == pre_fruit.begin() || itr == prev(pre_fruit.end())) continue;

            if (!CONTAINS(matches, config[c]))
                matches[config[c]] = TknvalsRegion { itr, next(itr) };
                        
            if (c+1 < config.size() && config[c+1] == *itr) {
                matches[config[c+1]] = TknvalsRegion { itr, next(itr) };
                                
                done = false;
                matches[config[c]].end = itr;
                c++; itr++;
                while (config[c] == *itr) {
                    matches[config[c]] = TknvalsRegion { itr, next(itr) };
                    c++; itr++;
                }
                c++;
                itr--;
            }
        }

        matches[config.back()].end = prev(pre_fruit.end());

        if (done) break;

        vector<string> new_pre_fruit = {};
        for (string t : to) {
            if (is_all_upper(t)) {
                copy(matches[t].beg, matches[t].end, back_inserter(new_pre_fruit));
            } else {
                new_pre_fruit.push_back(t);
            }
        }

        pre_fruit = new_pre_fruit;
                    
        done = true;
    }
}

inline bool is_notation_variable(const string& s) {
    bool res = true;
    for (char c : s) if (!isupper(c)) res = false;
    return res;
}

inline bool is_notation_free_variable(const string& s) {
    bool res = true;
    for (int i = 0; i < s.size()-1; ++i)
        if (!isupper(s[i])) res = false;
    if (s.back() != '?') res = false;
    return res;
}

void
replace(vector<vector<string>> &matches,
        const vector<string>& config, 
        const vector<string>& to
        )
// TODO: nvが重複している可能性は...?    
{
    vector<string> res = {};

    int i = 0;
    for (string notvar : to) {
        if (is_notation_variable(notvar)) {
            // NOTE: ここではエラーおきないはず
            copy(
                matches[INDEXOF(config, notvar)].begin(),
                matches[INDEXOF(config, notvar)].end(),
                back_inserter(res));
        } else {
            res.push_back(notvar);
        }
        
        i++;
    }

    matches.erase(matches.begin(), matches.begin() + config.size()-1);
    matches[0] = res;

    if (matches.size() == 1) return;

    replace(matches, config, to);
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


void match(
    vector<vector<string>> &res,
    const vector<string>& base,
    const vector<string>& config,
    pair<string,int> exit_token = "",
    bool forward = true)
{
    if (exit_token.first == "") {
        int i = 0;
        for (auto c : config) {
            if (!is_notation_variable(c)) {
                exit_token = make_pair(c, i);
                break;
            }
            i++;
        }
    }

    if (forward) {
        vector<string> buf;
        int c = 0;
        for (int i = 0; i < base.size(); ++i) {
            if (base[i] == exit_token.first && i-exit_token.second <= 0) {
                for (int j = 0; j < exit_token.second; ++j)
                    res.push_back(base[j]);
                res.push_back(base[i]);
            }
            
            if (c+1 < config.size() && base[i] == config[c+1]) {
                res.push_back(buf);
                buf.clear();
                // TODO: 特殊文字が複数回連続する
                res.push_back({base[i]});
                c += 2;
            }
            else {
                buf.push_back(base[i]);
            }

            if (i == base.size()-1) {
                if (find(buf.begin(), buf.end(), exit_token) != buf.end()) {
                    match(res, buf, config, exit_token, forward);
                } else {
                    res.push_back(buf);
                }
            }
        }
    }
}




// void backward_replace(vector<string>& pre_fruit, vector<string>& config, vector<string>& to) {
//     pre_fruit = reverse(pre_fruit.begin(), pre_fruit.end());
//     config = reverse(config.begin(), config.end());


//     forward_replace(pre_fruit, config, to);


// }




pair<Code, ReductionFlow> S_reduction(Code code, ReductionFlow rf) {


    cout << "Reductioning ... " << endl;
    cout << code << endl;


    if (!code.l) {
        if (is_literal(code.lit.val)) return make_pair(code,rf);
        else {
            if (CONTAINS(rf.bind, code.lit.val)) {
                Code res = *ref(code.lit.val);
                while (!res.l) {
                    if (is_literal(res.lit.val)) return make_pair(res, rf);
                    res = *ref(res.lit.val);
                }
                code.l = res.l;
                code.lit = res.lit;
                copy(res.args.begin(), res.args.begin(), back_inserter(code.args));
                // Code res = reduction(*rf.bind[code.lit.val], rf).first;
                // res.args = code.args;
                // rf.bind[code.lit.val] = &res; // NOTE: 結果反映
                // c = res;
                // TODO: どうすれば...
            } else if (code.lit.val == "notation") {
                // 表記の書き換え
                // 書き換え規則を取得して、第三引数のコードをソース情報から書き換える。
                // 再度パースしてできたCodeのreductionを今回の結果とする
                // NOTE: 実行順序は関係あるの？まとめてやったら？何度もパースしたくないし
                if (code.args.size() != 3) {
                    cout << "[ERROR] notationの引数の数が違います";
                    return make_pair(make_int_litcode(0), rf); // NOTE: エラー値として
                }


                vector<string> config = vector<string>(code.args[0].srcbeg, code.args[0].srcend);
                rmvparen(config); // NOTE: (とか)は受け付けないのでn


                // TODO: validity check of configuration (splash off something like 'A B ; C')


                vector<string> to = vector<string>(code.args[1].srcbeg, code.args[1].srcend);
                //                rmvparen(to);
                // Code to_code;
                // {

                //     ParserFlow pf = {to, 0};
                //     to_code = ::code(pf);
                // }

                vector<string> codestr = vector<string>(code.args[2].srcbeg, code.args[2].srcend);
                rmvparen(codestr);


                // matching...
                vector<vector<string>> matches;
                pad_notation_config(config);
                match(matches, codestr, config, "");
                if (matches.size() >= config.size()) {
                    replace(matches, config, to);

                    // re-parse pre_fruit
                    ParserFlow pf = {matches[0], 0};
                    Code fruit = ::code(pf);
                
                    return make_pair(S_reduction(fruit, rf).first, rf);
                } else {
                    code = code.args[2].l->body;
                }
            } else if (code.lit.val == "add") {
                code.l = &add;
            } else if (code.lit.val == "negate") {
                code.l = &::negate;
            } else {
                cout << "[ERROR] '" << code.lit.val << "'というような名前は見つかりません" << endl;
            }
        }
    }

    for (int i = code.args.size()-1; i >= 0; i--) {
        code.args[i] = S_reduction(code.args[i], rf).first;
        rf.argstack.push(&code.args[i]);
    }

    for (string argname : code.l->argnames) {
        Code *arg;
        if (rf.argstack.empty()) {
            // TODO: 部分適用 ? 
            cout << "[ERROR] 引数の数が足りません" << endl;
            return make_pair(code, rf);
        } else {
            arg = rf.argstack.top();
            rf.argstack.pop();
        }

        if (CONTAINS(rf.bind, argname)) {
            rf.shadower[argname] = random_string(16);
            // TODO: 衝突!
            rf.bind[rf.shadower[argname]] = arg;
        } else
            rf.bind[argname] = arg;
    }

    if (code.lit.val == "add") {
        pair<int,bool> tmp1 = read_int_litcode(ref("a1"));
        pair<int,bool> tmp2 = read_int_litcode(ref("a2"));
            
        if (tmp1.second && tmp2.second) 
            return make_pair(make_int_litcode(tmp1.first + tmp2.first), rf);
        else
            return make_pair(code, rf);
    } else if (code.lit.val == "negate") {
        pair<int,bool> tmp1 = read_int_litcode(ref("a1"));
            
        if (tmp1.second) 
            return make_pair(make_int_litcode(-tmp1.first), rf);
        else
            return make_pair(code, rf);
    }

    if (!code.l->body.l && code.l->body.lit.val == "")
        return make_pair(code, rf);
    
    return make_pair(S_reduction(code.l->body,rf).first, rf);
}



pair<Code, ReductionFlow> reduction(Code code, ReductionFlow rf) {

    ReductionFlow oldrf = rf;

    cout << "Reductioning ... " << endl;
    cout << code << endl;

    if (!code.l) {
        if (is_literal(code.lit.val)) return make_pair(code,rf);
        else {
            if (CONTAINS(rf.bind, code.lit.val)) {
                Code res = *ref(code.lit.val);
                while (!res.l) {
                    if (is_literal(res.lit.val)) return make_pair(res, rf);
                    res = *ref(res.lit.val);
                }
                code.l = res.l;
                code.lit = res.lit;
                copy(res.args.begin(), res.args.begin(), back_inserter(code.args));
                // Code res = reduction(*rf.bind[code.lit.val], rf).first;
                // res.args = code.args;
                // rf.bind[code.lit.val] = &res; // NOTE: 結果反映
                // c = res;
                // TODO: どうすれば...
            } else if (code.lit.val == "add") {
                code.l = &add;
            } else if (code.lit.val == "negate") {
                code.l = &::negate;
            } else {
                cout << "[ERROR] '" << code.lit.val << "'というような名前は見つかりません" << endl;
            }
        }
    }

    for (int i = code.args.size()-1; i >= 0; i--) {
        rf.argstack.push(&code.args[i]);
    }

    for (string argname : code.l->argnames) {
        Code *arg;
        if (rf.argstack.empty()) {
            // TODO: 部分適用 ? 
            cout << "[ERROR] 引数の数が足りません" << endl;
            return make_pair(code, rf);
        } else {
            arg = rf.argstack.top();
            rf.argstack.pop();
        }

        // if (CONTAINS(rf.bind, argname)) {
        //     rf.shadower[argname] = random_string(16);
        //     // TODO: 衝突!
        //     rf.bind[rf.shadower[argname]] = arg;
        // } else
        rf.bind[argname] = arg;
    }

    if (code.lit.val == "add") {
        Code a1 = reduction(*ref("a1"),rf).first;
        Code a2 = reduction(*ref("a2"),rf).first;
        pair<int,bool> tmp1 = read_int_litcode(a1);
        pair<int,bool> tmp2 = read_int_litcode(a2);
            
        if (tmp1.second && tmp2.second) 
            return make_pair(make_int_litcode(tmp1.first + tmp2.first), rf);
        else
            return make_pair(Code {&add, "add", code.args}, rf); // NOTE: ここのliteralを入れておくことでbodyが無いことを示せる
    } else if (code.lit.val == "negate") {
        Code a1 = reduction(*ref("a1"),rf).first;
        pair<int,bool> tmp1 = read_int_litcode(a1);
            
        if (tmp1.second) 
            return make_pair(make_int_litcode(-tmp1.first), rf);
        else
            return make_pair(Code {&::negate, "negate", code.args}, rf);
    }

    if (!code.l->body.l && code.l->body.lit.val == "")
        return make_pair(code, rf);
    
    return make_pair(reduction(code.l->body,rf).first, rf);
}


Code reduction(Code code, bool silent) {
    auto back = cout.rdbuf();
    if (silent) {
        cout.rdbuf(NULL);
    }

    ReductionFlow rf = {};
    Code redu = S_reduction(code, rf).first;

    if (silent) {
        cout.rdbuf(back);
    }
    return redu;
}


// Code F_reduction(Code code, ReductionFlow rf) {
//     if (!code.l) return code;

//     ReductionFlow oldrf = rf;
//     for (Code arg : code.args) {
//         reduction(arg, oldrf);
//     }
// }
