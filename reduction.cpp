/* ========================================================================
   $File: reduction.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"

{! BUILTIN_1 <> name <> Lambda #name = { { "a1" }, {} }; Code c_#name (&::#name, Literal {"#name"}, vector<Code> {}, TknvalsRegion {}); !}
{! BUILTIN_2 <> name <> Lambda #name = { { "a1", "a2" }, {} }; Code c_#name (&::#name, Literal {"#name"}, vector<Code> {}, TknvalsRegion {}); !}

{- BUILTIN_1 <> negate -}
{- BUILTIN_2 <> add -}

struct Notation {
    TknvalsRegion config;
    Code to;
};

struct ReductionFlow {
    map<string, Code*> bind = {
        { "add", &c_add },
        { "negate", &c_negate },
    };
    stack<Code*> argstack;
    // NOTE: 未計算のままshadowingするために必要
    map<string, string> shadower;
    vector<Notation> notations;
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
static inline void rmvparen(TknvalsRegion &r) {
    if (distance(r.beg, r.end) == 0) return;

    if (*r.beg == "(") r.beg++;
    if (*r.end == ")") r.end--;
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
    return Code (0, Literal { to_string(n) });
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

Code replace_code(Code c, const map<string, Code*> &d) {
    if (c.l) {
        for (string &a : c.l->argnames) {
            if (CONTAINS(d, a)) {
                a = d.at(a)->lit.val; // ?
            }
        }

        replace_code(c.l->body, d);
    } else if (c.lit.val != "") {
        if (CONTAINS(d, c.lit.val)) {
            c = *d.at(c.lit.val);
        }
    }

    for (auto &a : c.args) {
        a = replace_code(a, d);
    }

    return c;
}


inline void apply_notation(Code &code, const Notation& notation) {
    // first, check length
    if (code.args.size()+1 < notation.config.size())
        return;


    vector<Code> tmps;
    map<string, Code*> d;
    if (!is_notation_variable(*notation.config.beg) && code.lit.val != *notation.config.beg) return;
    d.insert(make_pair(*notation.config.beg, &code));


    int i = 0;
    for (auto i_notval = next(notation.config.beg);
         i_notval != notation.config.end;
         i_notval++)
    {
        // NOTE: 大きさのチェックは最初にしたのでこのときのみがチェック対象
        if (i >= code.args.size()) break;

        // free variable
        // 文字列を集めてパースただし、関数が潜んでいた場合... = add (f 3) 2;とか
        if (is_notation_free_variable(*i_notval)) {
            int start = i;
            tmps.push_back(code.args[i]);
            i++;
            while (true) {
                if (i >= code.args.size()
                    || 
                    code.args[i].lit.val == *next(i_notval))
                {
                    d.insert(make_pair(*i_notval, &tmps[tmps.size()-1]));
                    i--;
                    break;
                }

                tmps[tmps.size()-1].args.push_back(code.args[i]);
                tmps[tmps.size()-1].src.end = code.args[i].src.end;
                
                i++;
            }
        } else {
            if (!is_notation_variable(*i_notval) && code.args[i].lit.val != *i_notval) return;
            d.insert(make_pair(*i_notval, &code.args[i]));
        }
        i++;
    }

    Code fruit = notation.to;

    fruit = replace_code(fruit, d);
}

pair<Code, ReductionFlow> S_reduction(Code code, ReductionFlow rf) {

    cout << "Reductioning ... " << endl;
    cout << code << endl;


    for (const auto n : rf.notations) {
        apply_notation(code, n);
    }


    if (!code.l) {
        if (is_literal(code.lit.val)) return make_pair(code,rf);
        else {
            if (CONTAINS(rf.bind, code.lit.val)) {
                Code *res = ref(code.lit.val);
                while (!res->l) {
                    if (is_literal(res->lit.val)) return make_pair(*res, rf);
                    res = ref(res->lit.val);
                }
                code.l = res->l;
                code.lit = res->lit;
                copy(res->args.begin(), res->args.begin(), back_inserter(code.args));
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


                // TODO: validity check of configuration (splash off something like 'A B ; C')


                vector<string> to = vector<string>(code.args[1].src.beg, code.args[1].src.end);
                rmvparen(to);
                Code to_code;
                {

                    ParserFlow pf = {to, 0};
                    to_code = ::code(pf);
                }

                TknvalsRegion conf = code.args[0].src;
                rmvparen(conf);

                rf.notations.push_back(Notation {conf, to_code});

                return make_pair(S_reduction(code.args[2],rf).first, rf);
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
            return make_pair(Code (&add, Literal{"add"}, code.args), rf); // NOTE: ここのliteralを入れておくことでbodyが無いことを示せる
    } else if (code.lit.val == "negate") {
        Code a1 = reduction(*ref("a1"),rf).first;
        pair<int,bool> tmp1 = read_int_litcode(a1);
            
        if (tmp1.second) 
            return make_pair(make_int_litcode(-tmp1.first), rf);
        else
            return make_pair(Code (&::negate, Literal{"negate"}, code.args), rf);
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
