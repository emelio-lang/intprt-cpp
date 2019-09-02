/* ========================================================================
   $File: reduction.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"
#include "notation.h"

int ReductionCounter = 0;

{! BUILTIN_1 <> name <> shared_ptr<Lambda> #name_p = std::make_shared<Lambda>(Lambda { {"a1"}, {} }); Code c_#name {#name_p, Literal {"#name"}, vector<shared_ptr<Code>> {}, TknvalsRegion {}}; !}
{! BUILTIN_2 <> name <> shared_ptr<Lambda> #name_p = std::make_shared<Lambda>(Lambda { {"a1", "a2"}, {} }); Code c_#name {#name_p, Literal {"#name"}, vector<shared_ptr<Code>> {}, TknvalsRegion {}}; !}

{- BUILTIN_1 <> negate -}
{- BUILTIN_2 <> add -}
{- BUILTIN_2 <> concat -}

struct ReductionFlow {
    map<string, shared_ptr<Code>> bind = {
        { "add", make_shared<Code>(c_add) },
        { "negate", make_shared<Code>(c_negate) },
        { "concat", make_shared<Code>(c_concat) },
    };
    stack<shared_ptr<Code>> argstack;
    // NOTE: 未計算のままshadowingするために必要
    map<string, string> shadower;
    vector<Notation> notations;
    vector<Notation> greedy_notations;
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
    if (*prev(r.end) == ")") r.end--;
}


bool check_computed(const shared_ptr<Code> c) {
    return (c && !c->l);
}

pair<int,bool> read_int_litcode(const shared_ptr<Code> c) {
    if (!check_computed(c)) {
        if (c) {
            cout << "[ERROR] (TODO:カインド) 次のようなCodeをIntリテラルとして読もうとしました" << endl;
            cout << *c << endl;
        }
        return make_pair(0, false);
    } 

    return make_pair(stoi(c->lit.val), true);
}

pair<string,bool> read_string_litcode(const shared_ptr<Code> c) {
    if (!check_computed(c)) {
        if (c) {
            cout << "[ERROR] (TODO:カインド) 次のようなCodeをStringリテラルとして読もうとしました" << endl;
            cout << *c << endl;
        }
        return make_pair("", false);
    } 

    return make_pair(c->lit.val, true);
}

// pair<int,bool> read_int_litcode(const Code& c) {
//     if (!check_computed(&c)) {
//         cout << "[ERROR] (TODO:カインド) 次のようなCodeをIntリテラルとして読もうとしました" << endl;
//         cout << c << endl;
//         return make_pair(0, false);
//     } 

//     return make_pair(stoi(c.lit.val), true);
// }

// unique_ptr<Code> make_int_litcode(int n) {
//     unique_ptr<Code> res (make_unique<Code>(Code {0, Literal { to_string(n) }}));
//     return res;
// }
Code make_int_litcode(int n) {
    return Code {0, Literal { to_string(n) }};
}

Code make_string_litcode(string n) {
    return Code {0, Literal { n }};
}

// unique_ptr<Code> make_int_litcode(int n) {
//     unique_ptr<Code> res (make_unique<Code>(Code {0, Literal { to_string(n) }}));
//     return res;
// }

shared_ptr<Code> ref(string name, ReductionFlow &rf) {
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


ReductionFlow S_reduction(shared_ptr<Code> code, ReductionFlow rf) {
    shared_ptr<Code> given_p = code;
    
    while ( true ) {

        cout << "Reductioning ... " << endl;
        cout << *code << endl << endl;
        ReductionCounter++;

        // Apply notations
        {
            bool reapply = false;
            {
                auto n = rf.greedy_notations.begin();
                while (n != rf.greedy_notations.end()) {
                    reapply = apply_notation_greedily(code, *n);
                    n++;
                }
            }
            {
                auto n = rf.notations.begin();
                while (n != rf.notations.end()) {
                    reapply = apply_notation(code, *n);
                    n++;
                }
            }

            if (reapply) continue;
        }
        // {
        //     auto n = rf.notations.end();
        //     while (n != rf.notations.begin()) {
        //         n--;
        //         apply_notation(code, *n);
        //     } 
        // }

        if (!code->l) {
            if (is_literal(code->lit.val)) {
                *given_p = *code;
                return rf;
            } else {
                if (CONTAINS(rf.bind, code->lit.val)) {
                    // NOTE: while内refでの代入時にちゃんと切り替わるため
                    Code res = *ref(code->lit.val);
                    while (!res.l) {
                        if (is_literal(res.lit.val)) {
                            *given_p = res;
                            return rf;
                        }
                        res = *ref(res.lit.val); // DEBUG: check its counter ここでは消されないはず
                    }

                    if (res.l) {
                        if (!code->l)
                            code->l = shared_ptr<Lambda>(new Lambda);
                        code->l->deep_copy_from(*res.l);
                    } else res.l = shared_ptr<Lambda>(nullptr);
                    code->lit = res.lit;
                    // copy(res.args.begin(), res.args.begin(), back_inserter(code->args)); //?

            } else if (code->lit.val == "notation" || code->lit.val == "gnotation") {
                    // 表記の書き換え
                    // 書き換え規則を取得して、第三引数のコードをソース情報から書き換える。
                    // 再度パースしてできたCodeのreductionを今回の結果とする
                    // NOTE: 実行順序は関係あるの？まとめてやったら？何度もパースしたくないし
                    if (code->args.size() != 3) {
                        cout << "[ERROR] notationの引数の数が違います";
                        *given_p = make_int_litcode(0);
                        return rf; // NOTE: エラー値として
                    }


                    // TODO: validity check of configuration (splash off something like 'A B ; C')


                    vector<string> to = vector<string>(code->args[1]->src.beg, code->args[1]->src.end);
                    rmvparen(to);
                    shared_ptr<Code> to_code;
                    {

                        ParserFlow pf = {to, 0};
                        to_code = ::code(pf);
                    }

                    TknvalsRegion conf = code->args[0]->src;
                    rmvparen(conf);

                    if (code->lit.val == "notation") {
                    rf.notations.push_back(Notation {conf, to_code});
                    } else if (code->lit.val == "gnotation") {
                    rf.greedy_notations.push_back(Notation {conf, to_code});
                    }

                    // DEBUG
                    *given_p = *code->args[2];
                    return S_reduction(given_p, rf);
                } else if (code->lit.val == "add") {
                    code->l.reset(add_p.get());
                } else if (code->lit.val == "negate") {
                    code->l.reset(negate_p.get());
                } else if (code->lit.val == "concat") {
                    code->l.reset(concat_p.get());
                } else if (code->lit.val == "nothing") {
                    
                } else {
                    cout << "[ERROR] '" << code->lit.val << "'というような名前は見つかりません" << endl;
                    *given_p = *code;
                    return rf;
                }
            }
        }

        for (int i = code->args.size()-1; i >= 0; i--) {
            S_reduction(code->args[i], rf); // TODO: ここのrfいらない？
            rf.argstack.push(code->args[i]);
        }

        for (string argname : code->l->argnames) {
            if (rf.argstack.empty()) {
                // TODO: 部分適用 ? 
                cout << "[ERROR] 引数の数が足りません" << endl;
                return rf;
            } else {
                rf.bind[argname] = rf.argstack.top();
                rf.argstack.pop();
            }
        }

        if (code->lit.val == "add") {
            pair<int,bool> tmp1 = read_int_litcode(ref("a1"));
            pair<int,bool> tmp2 = read_int_litcode(ref("a2"));

            
            if (tmp1.second && tmp2.second) {
                cout << "add " << tmp1.first << " " << tmp2.first << endl << endl;
                *given_p = make_int_litcode(tmp1.first + tmp2.first);
                return rf;
            } else {
                *given_p = *code;
                return rf;
            }
        } else if (code->lit.val == "negate") {
            pair<int,bool> tmp1 = read_int_litcode(ref("a1"));
            
            if (tmp1.second) {
                cout << "negate " << tmp1.first << endl << endl;
                *given_p  = make_int_litcode(-tmp1.first);
                return rf;
            } else {
                *given_p = *code;
                return rf;
            }
        } else if (code->lit.val == "concat") {
            pair<string,bool> tmp1 = read_string_litcode(ref("a1"));
            pair<string,bool> tmp2 = read_string_litcode(ref("a2"));
            
            if (tmp1.second && tmp2.second) {
                cout << "concat " << tmp1.first << " " << tmp2.first << endl << endl;
                *given_p = make_string_litcode(tmp1.first + tmp2.first);
                return rf;
            } else {
                *given_p = *code;
                return rf;
            }
        }

        if (!code->l->body->l && code->l->body->lit.val == "") {
            *given_p = *code;
            return rf;
        }

        code = code->l->body; // DEBUG!
    }
    return rf;
}


void reduction(shared_ptr<Code> code, bool silent) {
    auto back = cout.rdbuf();
    if (silent) {
        cout.rdbuf(NULL);
    }

    ReductionFlow rf = {};
    S_reduction(code, rf);

    cout << ReductionCounter << " reductions." << endl;

    if (silent) {
        cout.rdbuf(back);
    }
}
