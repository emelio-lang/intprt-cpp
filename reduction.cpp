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

enum NotationType { LEFT, RIGHT };

struct ReductionFlow {
    map<string, shared_ptr<Code>> bind = {
        { "add", make_shared<Code>(c_add) },
        { "negate", make_shared<Code>(c_negate) },
        { "concat", make_shared<Code>(c_concat) },
    };
    stack<shared_ptr<Code>> argstack;
    // NOTE: 未計算のままshadowingするために必要
    map<string, string> shadower;
    vector<pair<Notation, NotationType>> notations;
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

pair<int,bool> read_int_litcode(const shared_ptr<Code> c) {
    if (!is_computed(c)) {
        if (c) {
            cout << "[ERROR] (TODO:カインド) 次のようなCodeをIntリテラルとして読もうとしました" << endl;
            cout << *c << endl;
        }
        return make_pair(0, false);
    } 

    return make_pair(stoi(c->lit.val), true);
}

pair<string,bool> read_string_litcode(const shared_ptr<Code> c) {
    if (!is_computed(c)) {
        if (c) {
            cout << "[ERROR] (TODO:カインド) 次のようなCodeをStringリテラルとして読もうとしました" << endl;
            cout << *c << endl;
        }
        return make_pair("", false);
    } 

    return make_pair(c->lit.val, true);
}

// unique_ptr<Code> make_procedural_code(shared_ptr<Code> from, shared_ptr<Code> to) {
//     return make_unique<Code>(Code {
//             make_shared<Lambda>(Lambda{{"_"}, to}),
//             "",
//             { from },
//             TknvalsRegion { from.src.beg, to.src.end } // TODO: これ大丈夫？
//         });
// }

Code make_int_litcode(int n) {
    return Code {0, Literal { to_string(n) }};
}

Code make_string_litcode(string n) {
    return Code {0, Literal { n }};
}

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


bool apply_all_notations(shared_ptr<Code> &code,
                         const vector<pair<Notation,NotationType>> &notations,
                         bool rv = false
                         )
{
    bool reapply = false;
    {
        auto n = notations.begin();
        while (n != notations.end()) {
            switch (n->second) {
                case RIGHT:
                    reapply = apply_notation(code, n->first, true);
                    break;
                case LEFT:
                    reapply = apply_notation(code, n->first);
                    break;
            }
            n++;
        }
    }

    if (reapply) apply_all_notations(code, notations, true);
    return rv;
}


void resolve_fusion(shared_ptr<Code> &code, const ReductionFlow &rf) {
    stack<shared_ptr<Code>> _argstack = rf.argstack;
    // 0, 1を残してargがあるなら降順に詰めていく
    // for (auto arg = prev(code->args.end());
    //      std::distance(code->args.begin(), arg) > 1;
    //      arg--)
    // {
    //     _argstack.push(*arg);
    // }
    
    int go1 = 0, go2 = 0;
    bool go1_proh = false, go2_proh = false;
    for (int i = 0; i < code->args[0]->l->argnames.size(); ++i) {
        string argname1 = code->args[0]->l->argnames[i];
        string argname2 = code->args[1]->l->argnames[i];

        if (is_number(argname1)) {
            if (argname1 == _argstack.top()->lit.val)
                go1++;
            else
                go1_proh = true;
        }
        if (is_number(argname2)) {
            if (argname2 == _argstack.top()->lit.val)
                go2++;
            else
                go2_proh = true;
        }
    }

    if (go1_proh && go2_proh) {
        cerr << "[ERROR] fusionの解決が出来ません" << endl;
        cerr << *code << endl;
    } else if (!go1_proh && !go2_proh) {
        if (go1 == go2) {
            cerr << "[ERROR] fusionの解決が出来ません" << endl;
            cerr << *code << endl;
        } else if (go1 < go2) {
            code = code->args[1];
        } else if (go2 < go1) {
            code = code->args[0];
        }
    }
    else if (!go1_proh) code = code->args[0];
    else if (!go2_proh) code = code->args[1];
}


ReductionFlow S_reduction(shared_ptr<Code> code, ReductionFlow rf) {
    shared_ptr<Code> given_p = code;
    
    while ( true ) {
continue_reduction_loop:

        cout << "Reductioning ... " << endl;
        cout << *code << endl << endl;
        ReductionCounter++;

        // Apply notations
        apply_all_notations(code, rf.notations);
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

                        // NOTE: つまり前の階層と同じようなことをする
                        // ここのcontinueは例えばfuseとかはbindにないので、continueすることで再ロードできる
                        // TODO: fuseの場合はこれじゃ駄目だけど、とりあえず一貫性を保っておく
                        if (!CONTAINS(rf.bind, res.lit.val)) {
                            *given_p = res;
                            return rf;
                        }
                        res = *ref(res.lit.val); // DEBUG: check its counter ここでは消されないはず
                    }

                    // deep copy res's lambda
                    if (res.l) {
                        if (!code->l)
                            code->l = shared_ptr<Lambda>(new Lambda);
                        code->l->deep_copy_from(*res.l);
                    } else res.l = shared_ptr<Lambda>(nullptr);
                    code->lit = res.lit;
                    for (const shared_ptr<Code> &arg : res.args) {
                        code->args.push_back(make_shared<Code>(*arg));
                    }
                    
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


                    // vector<string> to = vector<string>(code->args[1]->src.beg, code->args[1]->src.end);
                    // rmvparen(to);
                    shared_ptr<Code> to_code = make_shared<Code>(*code->args[1]);
                    // {

                    //     ParserFlow pf = {to, 0};
                    //     to_code = ::code(pf);
                    // }

                    vector<string> conf = code->args[0]->plain_string();
//                    rmvparen(conf);

                    if (code->lit.val == "notation") {
                        rf.notations.push_back(make_pair(Notation {conf, to_code}, LEFT));
                    } else if (code->lit.val == "gnotation") {
                        rf.notations.push_back(make_pair(Notation {conf, to_code}, RIGHT));
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
                } else if (code->lit.val == "fuse") {
                    // fusionの解決
                    // fusionされている関数の引数の数は一緒であるはずなので、
                    // どちらの関数か決定できれば解決される
                    if (rf.argstack.size() >= code->args[0]->l->argnames.size()) {
                        resolve_fusion(code, rf);
                        continue;
                    }
                    else
                    {
                        *given_p = *code;
                        return rf;
                    }
                } else if (code->lit.val == "nothing") {
                    
                } else {
                    cout << "[ERROR] '" << code->lit.val << "'というような名前は見つかりません" << endl;
                    *given_p = *code;
                    return rf;
                }
            }
        }

        ReductionFlow oldrf = rf;
        for (int i = code->args.size()-1; i >= 0; i--) {
            S_reduction(code->args[i], oldrf); // TODO: ここのrfいらない？
            rf.argstack.push(code->args[i]);
        }

        // if (code->lit.val == "fuse" &&
        //     (rf.argstack.size()+maxzero((int)code->args.size()-2)) >= code->args[0]->l->argnames.size())
        // {
        //     resolve_fusion(code, rf);
        //     continue;
        // }

        for (string argname : code->l->argnames) {
            if (rf.argstack.empty()) {
                // TODO: 部分適用 ? 
                cout << "[ERROR] 引数の数が足りません" << endl;
                return rf;
            } else {
                if (argname != "_") {
                    rf.bind[argname] = rf.argstack.top();
                }
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
        } else if (code->lit.val == "fuse") {
            // TODO: a1 とか a2 とか必要？
            shared_ptr<Code> tmp1 = ref("a1");
            shared_ptr<Code> tmp2 = ref("a2");
            
            cout << "fuse " << tmp1 << " " << tmp2 << endl << endl;

            

            if (// is_fusable(tmp1) && is_fusable(tmp2) && 
                tmp1->l->argnames.size() == tmp2->l->argnames.size() ) {
                *given_p = *code;
                return rf;
            } else {
                cerr << "[ERROR] fusableじゃないのでどうにかしよう" << endl;
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


ReductionFlow extract_all_notations(shared_ptr<Code> &code, ReductionFlow rf) {
    cout << *code << endl << endl;

    // これはまだ続くのかどうか TODO 一貫性
//    if (check_all_notations(code, rf.notations, rf.greedy_notations)) return rf;


    // Apply notations
    apply_all_notations(code, rf.notations);

    if (!code->l) {
        if (is_literal(code->lit.val)) {
//                *given_p = *code;
            return rf;
        } else {
            if (code->lit.val == "notation" || code->lit.val == "gnotation") {
                // 表記の書き換え
                // 書き換え規則を取得して、第三引数のコードをソース情報から書き換える。
                // 再度パースしてできたCodeのreductionを今回の結果とする
                // NOTE: 実行順序は関係あるの？まとめてやったら？何度もパースしたくないし
                if (code->args.size() != 3) {
                    cout << "[ERROR] notationの引数の数が違います";
                    return rf; // NOTE: エラー値として
                }


                // TODO: validity check of configuration (splash off something like 'A B ; C')


                // vector<string> to = vector<string>(code->args[1]->src.beg, code->args[1]->src.end);
                // rmvparen(to);
                shared_ptr<Code> to_code = make_shared<Code>(*code->args[1]);
                // {

                //     ParserFlow pf = {to, 0};
                //     to_code = ::code(pf);
                // }

                vector<string> conf = code->args[0]->plain_string();
                for (auto e : conf) cout << e << " "; 
                cout << endl;
//                rmvparen(conf);

                if (code->lit.val == "notation") {
                    rf.notations.push_back(make_pair(Notation {conf, to_code}, LEFT));
                } else if (code->lit.val == "gnotation") {
                    rf.notations.push_back(make_pair(Notation {conf, to_code}, RIGHT));
                }

                cout << "There now notations like: \n";
                for (auto notation : rf.notations) {
                    for (auto i_notval = notation.first.config.begin();
                         i_notval != notation.first.config.end();
                         i_notval++)
                        cout << *i_notval << " ";
                    cout << endl;
                }
                cout << endl;


                // DEBUG
                // TODO: ここで前のやつと繋げないといけなさそう
                // 先に引数の方を展開して、手続き結合をして返す
                extract_all_notations(code->args[2], rf);
                code = code->args[2];
//                    *code = *code->args[2];
                return rf;
            }

            for (int i = code->args.size()-1; i >= 0; i--) {
                extract_all_notations(code->args[i], rf);
            }

            return rf;
        }
    }

    for (int i = code->args.size()-1; i >= 0; i--) {
//        if (check_all_notations(code->args[i], rf.notations, rf.greedy_notations)) {
            extract_all_notations(code->args[i], rf);
//        }
    }

    if (!code->l->body->l && code->l->body->lit.val == "") {
//            *given_p = *code;
        return rf;
    }

    extract_all_notations(code->l->body, rf);
    return rf;
}


void extract_all_notations(shared_ptr<Code> code, bool silent) {
    auto back = cout.rdbuf();
    if (silent) {
        cout.rdbuf(NULL);
    }

    ReductionFlow rf = {};
    extract_all_notations(code, rf);

    if (silent) {
        cout.rdbuf(back);
    }
}

