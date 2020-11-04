#include "util.h"
#include "emelio.h"
#include <sstream>


TypeSignature arg(const TypeSignature &typesig, int n) {
    ASSERT(MATCHS(TypeFn)(typesig), "関数型でしか呼べない関数 arg ( " + to_string(typesig) + ", " + to_string(n) + " )");
    auto fn = PURES(TypeFn)(typesig);
    ASSERT(n < fn->from.size(), "ない引数を取得しようとしています arg ( " + to_string(typesig) + ", " + to_string(n) + " )");
    return fn->from[n];
}


TypeSignature sumfactor(const shared_ptr<TypeSum> &sumtype) {
    TypeSignature res = shared_ptr<TypeFn>(new TypeFn);
    auto fn = PURES(TypeFn)(res);
    deep_copy_from(fn->to, PURES(TypeFn)(sumtype->sums[0])->to);
    for (int i = 0; i < arity(sumtype->sums[0]); i++) {
        TypeSignature new_sumtype = shared_ptr<TypeSum>(new TypeSum);
        auto sum = PURES(TypeSum)(new_sumtype);
        for (int j = 0; j < sumtype->sums.size(); j++) {
            sum->sums.emplace_back();
            deep_copy_from(sum->sums.back(), PURES(TypeFn)(sumtype->sums[j])->from[i]);
        }
        fn->from.emplace_back(new_sumtype);
    }
    return res;
}

void apply(TypeSignature &typesig, const int _n) {
    if (_n <= 0) return;
    ASSERT(MATCHS(TypeFn)(typesig), "関数型にしか適用できません apply ( " + to_string(typesig) + ", " + to_string(_n) + " )");

    int n = _n;
    auto fn = PURES(TypeFn)(typesig);
    for (int i = 0; i < _n; i++) {
        if (fn->from.size() == 0) {
            typesig = fn->to;
            apply(typesig, n);
            break;
        }
        fn->from.pop_front();
        n--;
    }

    if (fn->from.size() == 0)
        typesig = fn->to;
}

void wrap(TypeSignature &typesig, const TypeSignature &wrp) {
    TypeSignature ts;
    deep_copy_from(ts, wrp);
//    cout << to_string(typesig) << " " << to_string(wrp) << endl;
    if (MATCHS(TypeFn)(typesig)) {
        auto fn = PURES(TypeFn)(typesig);
        fn->from.push_front(ts);
    } else {
        TypeSignature newts = shared_ptr<TypeFn>(new TypeFn);
        auto newfn = PURES(TypeFn)(newts);
        newfn->from.push_back(ts);
        deep_copy_from(newfn->to, typesig);
        typesig = newts;
    }
}


// A->(B->C)   ====>  A-B->C
void _normalize_fn_fn(TypeSignature &typesig) {
    if (MATCHS(TypeFn)(typesig)) {
        auto outer_fn = PURES(TypeFn)(typesig);
        _normalize_fn_fn(outer_fn->to);
        for (auto &e : outer_fn->from) {
            _normalize_fn_fn(e);
        }

        if (MATCHS(TypeFn)(outer_fn->to)) {
            auto to_fn = PURES(TypeFn)(outer_fn->to);
            TypeFn new_fntype;
            deep_copy_from(new_fntype.to, to_fn->to);
            for (int i = 0; i < outer_fn->from.size(); i++) {
                new_fntype.from.emplace_back(shared_ptr<TypeFn>(nullptr));
                deep_copy_from(new_fntype.from.back(), outer_fn->from[i]);
            }
            for (int i = 0; i < to_fn->from.size(); i++) {
                new_fntype.from.emplace_back(shared_ptr<TypeFn>(nullptr));
                deep_copy_from(new_fntype.from.back(), to_fn->from[i]);
            }
            typesig = make_shared<TypeFn>(new_fntype);
        }
    } else if (MATCHS(TypeSum)(typesig)) {
        auto outer_fn = PURES(TypeSum)(typesig);
        for (auto &e : outer_fn->sums) {
            _normalize_fn_fn(e);
        }
    } else if (MATCHS(TypeProduct)(typesig)) {
        auto outer_fn = PURES(TypeProduct)(typesig);
        for (auto &e : outer_fn->products) {
            _normalize_fn_fn(e);
        }
    } else if (MATCH(string)(typesig)) {
    } else if (MATCH(SpecialValue)(typesig)) {
    } else if (MATCHS(Parametered)(typesig)) {
    }
}

// (A|B)->C   ====>  (A->C|B->C)
void _normalize_fn_sum(TypeSignature &typesig) {
    // (A1|A2|A3|...) -> C
    if (MATCHS(TypeFn)(typesig)) {
        auto outer_fn = PURES(TypeFn)(typesig);
        _normalize_fn_sum(outer_fn->to);
        for (auto &e : outer_fn->from) {
            _normalize_fn_sum(e);
        }

        for (int i = outer_fn->from.size()-1; i >= 0; --i) {
            auto &from = outer_fn->from[i];
//            cout << i << endl;
            if (MATCHS(TypeSum)(from)) {
                auto codom_sum = PURES(TypeSum)(from);
                TypeSum new_sumtype;
        
                for (const auto &e : codom_sum->sums) {
                    from = e;
                    TypeSignature tmp = shared_ptr<TypeFn>(new TypeFn);
                    deep_copy_from(tmp, outer_fn);
                    new_sumtype.add_type(tmp);
                }
                typesig = make_shared<TypeSum>(new_sumtype);
                break;
            }
        }
        // A - B ... -> (A1 A2 A3 ...)
        // 
    } else if (MATCHS(TypeSum)(typesig)) {
        auto outer_fn = PURES(TypeSum)(typesig);
        for (auto &e : outer_fn->sums) {
            _normalize_fn_sum(e);
        }
    } else if (MATCHS(TypeProduct)(typesig)) {
        auto outer_fn = PURES(TypeProduct)(typesig);
        for (auto &e : outer_fn->products) {
            _normalize_fn_sum(e);
        }
    } else if (MATCH(string)(typesig)) {
    } else if (MATCH(SpecialValue)(typesig)) {
    } else if (MATCHS(Parametered)(typesig)) {
    }
}

// A->(B C)   ====>  (A->B A->C)
void _normalize_fn_prod(TypeSignature &typesig) {
    // (A1|A2|A3|...) -> C
    if (MATCHS(TypeFn)(typesig)) {
        auto outer_fn = PURES(TypeFn)(typesig);
        _normalize_fn_prod(outer_fn->to);
        for (auto &e : outer_fn->from) {
            _normalize_fn_prod(e);
        }

        if (MATCHS(TypeProduct)(outer_fn->to)) {
            auto dom_prod = PURES(TypeProduct)(outer_fn->to);
            TypeProduct new_prodtype;
        
            for (const auto &e : dom_prod->products) {
                outer_fn->to = e;
                new_prodtype.products.emplace_back(shared_ptr<TypeFn>(new TypeFn));
                deep_copy_from(new_prodtype.products.back(), outer_fn);
            }
            typesig = make_shared<TypeProduct>(new_prodtype);
        }
        // A - B ... -> (A1 A2 A3 ...)
        // 
    } else if (MATCHS(TypeSum)(typesig)) {
        auto outer_fn = PURES(TypeSum)(typesig);
        for (auto &e : outer_fn->sums) {
            _normalize_fn_prod(e);
        }
    } else if (MATCHS(TypeProduct)(typesig)) {
        auto outer_fn = PURES(TypeProduct)(typesig);
        for (auto &e : outer_fn->products) {
            _normalize_fn_prod(e);
        }
    } else if (MATCH(string)(typesig)) {
    } else if (MATCH(SpecialValue)(typesig)) {
    } else if (MATCHS(Parametered)(typesig)) {
    }
}

// (A (B|C)) =====> (A B) | (A C)
void _normalize_bunpai(TypeSignature &typesig) {
        // (A1|A2|A3|...) -> C
    if (MATCHS(TypeFn)(typesig)) {
        auto outer_fn = PURES(TypeFn)(typesig);
        _normalize_bunpai(outer_fn->to);
        for (auto &e : outer_fn->from) {
            _normalize_bunpai(e);
        }
    } else if (MATCHS(TypeSum)(typesig)) {
        for (auto &e : PURES(TypeSum)(typesig)->sums) {
            _normalize_bunpai(e);
        }
    } else if (MATCHS(TypeProduct)(typesig)) {
        // (A ...  (B|C)) について、逆からsumを見つけては((A ... B)|(A ... C))にする変形を行う
        auto outer_fn = PURES(TypeProduct)(typesig);
        TypeSignature res;
        for (int i = outer_fn->products.size()-1; i >= 0; i--) {
            auto e = outer_fn->products[i];
            _normalize_bunpai(e);
            if (MATCHS(TypeSum)(e)) {
                auto &target_sum = PURES(TypeSum)(e);
                res = shared_ptr<TypeSum>(new TypeSum);
                auto &resp = PURES(TypeSum)(res);

                for (int j = 0; j < target_sum->sums.size(); j++) {
                    resp->sums.emplace_back();
                    // NOTE: namesについてもこれでうまくいく
                    deep_copy_from(resp->sums.back(), typesig);
                    deep_copy_from(PURES(TypeProduct)(resp->sums.back())->products[i],
                                   target_sum->sums[j]);
                }
                typesig = res;
                break;
            }
        }
        
    } else if (MATCH(string)(typesig)) {    
    } else if (MATCH(SpecialValue)(typesig)) {    
    } else if (MATCHS(Parametered)(typesig)) {
    }
}

// (A | (B|C))   ====>  (A|B|C)
// (A (B C))   ====>  (A B C)
void _normalize_simile(TypeSignature &typesig) {
    // (A1|A2|A3|...) -> C
    if (MATCHS(TypeFn)(typesig)) {
        auto outer_fn = PURES(TypeFn)(typesig);
        _normalize_simile(outer_fn->to);
        for (auto &e : outer_fn->from) {
            _normalize_simile(e);
        }
    } else if (MATCHS(TypeSum)(typesig)) {
        auto outer_fn = PURES(TypeSum)(typesig);
        TypeSignature res = shared_ptr<TypeSum>(new TypeSum);
        auto &resp = PURES(TypeSum)(res);
        int i = 0;
        for (auto &e : outer_fn->sums) {
            _normalize_simile(e);
            if (MATCHS(TypeSum)(e)) {
                for (auto &j : PURES(TypeSum)(e)->sums) {
                    resp->add_type(j);
                    i++;
                }
            } else resp->add_type(e);
        }
        
        typesig = res;
    } else if (MATCHS(TypeProduct)(typesig)) {
        auto outer_fn = PURES(TypeProduct)(typesig);
        TypeSignature res = shared_ptr<TypeProduct>(new TypeProduct);
        auto &resp = PURES(TypeProduct)(res);
        int i = 0;
        for (auto &e : outer_fn->products) {
            _normalize_simile(e);
            if (MATCHS(TypeProduct)(e)) {
                int k = 0;
                for (auto &j : PURES(TypeProduct)(e)->products) {
                    resp->products.emplace_back(j);
                    resp->names.emplace_back(PURES(TypeProduct)(e)->names[i]);
                    k++;
                }
            } else {
                resp->products.emplace_back(e);
                resp->names.emplace_back(outer_fn->names[i]);
            }
            i++;
        }
        
        typesig = res;
    } else if (MATCH(string)(typesig)) {    
    } else if (MATCH(SpecialValue)(typesig)) {    
    } else if (MATCHS(Parametered)(typesig)) {
    }
}

void normalize(TypeSignature &typesig) {
    TypeSignature tmp;
    deep_copy_from(tmp, typesig);
    while (true) {
        _normalize_fn_fn(typesig);
        if (equal(tmp, typesig)) break;
        deep_copy_from(tmp, typesig);
    }
    while (true) {
        _normalize_fn_sum(typesig);
        if (equal(tmp, typesig)) break;
        deep_copy_from(tmp, typesig);
    }
    while (true) {
        _normalize_fn_prod(typesig);
        if (equal(tmp, typesig)) break;
        deep_copy_from(tmp, typesig);
    }
    while (true) {
        _normalize_bunpai(typesig);
        if (equal(tmp, typesig)) break;
        deep_copy_from(tmp, typesig);
    }
    while (true) {
        _normalize_simile(typesig);
        if (equal(tmp, typesig)) break;
        deep_copy_from(tmp, typesig);
    }
}

TypeSignature normalized(const TypeSignature &typesig) {
    TypeSignature res;
    deep_copy_from(res, typesig);
    normalize(res);
    return res;
}

// bool verify(const TypeSignature &ts1, const TypeSignature &ts2, const map<string,TypeSignature> &bind) {
//     if (equal(ts1,ts2,bind)) return true;
//     else return false;

//     if (MATCHS(TypeSum)(ts1)) {
//         for (int i = 0; i < PURES(TypeSum)(ts1)->sums.size(); ++i) {
//             bool tmp = false;
//             for (int j = 0; j < PURES(TypeSum)(ts2)->sums.size(); ++j) {
//                 tmp = tmp || equal(PURES(TypeSum)(ts1)->sums[i], PURES(TypeSum)(ts2)->sums[j], bind);
//             }
//             if (!tmp) return false;
//         }
//         return true;
//     } else {
//     }
// }


// bool equal(const TypeSignature &ts1, const TypeSignature &ts2) {
//     bool res = true;

//     if (MATCHS(TypeFn)(ts1) && MATCHS(TypeFn)(ts2)) {
//         cout << to_string(ts1) << " " << to_string(ts2) << endl;
//         res = res && equal(PURES(TypeFn)(ts1)->to, PURES(TypeFn)(ts2)->to);
//         cout << res << endl;
//         res = res && PURES(TypeFn)(ts1)->from.size() == PURES(TypeFn)(ts2)->from.size();
//         cout << res << endl;
//         for (int i = 0; i < PURES(TypeFn)(ts1)->from.size(); ++i) {
//             cout << res << endl;
//             res = res && equal(PURES(TypeFn)(ts1)->from[i], PURES(TypeFn)(ts2)->from[i]);
//         }
//         return res;
//     } else if (MATCHS(TypeSum)(ts1) && MATCHS(TypeSum)(ts2)) {
//         unordered_set<TypeSignature> a(PURES(TypeSum)(ts1)->sums.begin(), PURES(TypeSum)(ts1)->sums.end());
//         unordered_set<TypeSignature> b(PURES(TypeSum)(ts2)->sums.begin(), PURES(TypeSum)(ts2)->sums.end());
//         return a == b;
//     } else if (MATCHS(TypeProduct)(ts1) && MATCHS(TypeProduct)(ts2)) {
//         if (PURES(TypeProduct)(ts1)->products.size() != PURES(TypeProduct)(ts2)->products.size()) return false;
//         for (int i = 0; i < PURES(TypeProduct)(ts1)->products.size(); ++i) {
//             res = res && equal(PURES(TypeProduct)(ts1)->products[i], PURES(TypeProduct)(ts2)->products[i]);
//         }
//         return res;
//     } else if (MATCH(string)(ts1) && MATCH(string)(ts2)) {
//         return PURE(string)(ts1) == PURE(string)(ts2);
//     } else if (MATCHS(Parametered)(ts1) && MATCHS(Parametered)(ts2)) {
//         if (PURES(Parametered)(ts1)->params.size() != PURES(Parametered)(ts2)->params.size()) return false;
//         res = PURES(Parametered)(ts1)->type == PURES(Parametered)(ts2)->type;
//         for (int i = 0; i < PURES(Parametered)(ts1)->params.size(); i++) {
//             res = res && equal(PURES(Parametered)(ts1)->params[i], PURES(Parametered)(ts2)->params[i]);
//         }
//     } else return false;
// }
bool equal(const TypeSignature &ts1, const TypeSignature &ts2, const map<string,TypeSignature> &bind, int lvl) {
    bool res = true;

    if ((MATCH(string)(ts1) && PURE(string)(ts1) == "any") &&
        (MATCH(string)(ts2) && PURE(string)(ts2) == "any")) {
        return true;
    }

    // bool ts1_expandable = MATCH(string)(ts1) && bind.contains(PURE(string)(ts1));
    // bool ts2_expandable = MATCH(string)(ts2) && bind.contains(PURE(string)(ts2));
    // // 1のほうが、ただのstringで、newtypeとして定義されたものならば、それを一回だけ展開してそれを比較する
    // if ((ts1_expandable || ts2_expandable) && lvl < 2) {
    //     cout << "Expand" << endl;
    //     res = equal(ts1_expandable ? bind.at(PURE(string)(ts1)) : ts1,
    //                 ts2_expandable ? bind.at(PURE(string)(ts2)) : ts2,
    //                 bind, lvl+1);

    //     if (res)
    //         cout << "-> true" << endl;
    //     else
    //         cout << "-> false" << endl;
    //     return res;
    // }

    if (MATCHS(TypeFn)(ts1) && MATCHS(TypeFn)(ts2)) {
        cout << "Function-wise" << endl;
        res = equal(PURES(TypeFn)(ts1)->to, PURES(TypeFn)(ts2)->to, bind, lvl) && res;
        res = PURES(TypeFn)(ts1)->from.size() == PURES(TypeFn)(ts2)->from.size() && res;
        for (int i = 0; i < PURES(TypeFn)(ts1)->from.size(); ++i) {
            res = equal(PURES(TypeFn)(ts1)->from[i], PURES(TypeFn)(ts2)->from[i], bind, lvl) && res;
        }
    } else if (MATCHS(TypeProduct)(ts1) && MATCHS(TypeProduct)(ts2)) {
        cout << "Product-wise" << endl;
        if (PURES(TypeProduct)(ts1)->products.size() != PURES(TypeProduct)(ts2)->products.size()) return false;
        for (int i = 0; i < PURES(TypeProduct)(ts1)->products.size(); ++i) {
            res = equal(PURES(TypeProduct)(ts1)->products[i], PURES(TypeProduct)(ts2)->products[i], bind, lvl) && res;
        }
    } else if (MATCH(string)(ts1) && MATCH(string)(ts2)) {
        cout << "string" << endl;
        res = PURE(string)(ts1) == PURE(string)(ts2);
    } else if (MATCH(SpecialValue)(ts1) && MATCH(SpecialValue)(ts2)) {
        cout << "SpecialValue" << endl;
        res = PURE(SpecialValue)(ts1).val == PURE(SpecialValue)(ts2).val;
    } else if (MATCHS(Parametered)(ts1) && MATCHS(Parametered)(ts2)) {
        cout << "Parametered" << endl;
        if (PURES(Parametered)(ts1)->params.size() != PURES(Parametered)(ts2)->params.size()) return false;
        res = PURES(Parametered)(ts1)->type == PURES(Parametered)(ts2)->type;
        for (int i = 0; i < PURES(Parametered)(ts1)->params.size(); i++) {
            res = equal(PURES(Parametered)(ts1)->params[i], PURES(Parametered)(ts2)->params[i], bind, lvl) && res;
        }
    } else if (MATCHS(TypeSum)(ts1) && MATCHS(TypeSum)(ts2)) {
        res = true;
        for (auto s2 : PURES(TypeSum)(ts2)->sums) {
            bool tmp = false;
            for (auto s1 : PURES(TypeSum)(ts1)->sums) {
                cout << "1 sum-wise" << PURES(TypeSum)(ts1)->sums.size() << endl;
                if (equal(s1, s2, bind, lvl)) {
                    tmp = true;
                    break;
                }
            }
            res = res && tmp;
            if (!res) break;
        }
    // } else if () {
    //     res = false;
    //     for (auto s2 : PURES(TypeSum)(ts2)->sums) {
    //         cout << "2 sum-wise" << endl;
    //         res = equal(ts1, s2, bind, lvl) || res;
    //     }
    } else {
        res = false;
    }

    if (res)
        cout << "-> true" << endl;
    else
        cout << "-> false" << endl;

    return res;
}

bool verify(const TypeSignature &ts1, const TypeSignature &ts2, const map<string,TypeSignature> &bind, int lvl) {
    bool res = true;

    cout << to_string(ts1) << " " << to_string(ts2) << endl;
    if ((MATCH(string)(ts1) && PURE(string)(ts1) == "any") ||
        (MATCH(string)(ts2) && PURE(string)(ts2) == "any")) {
        cout << "-> true" << endl;
        return true;
    }

    bool ts1_expandable = MATCH(string)(ts1) && bind.contains(PURE(string)(ts1));
    bool ts2_expandable = MATCH(string)(ts2) && bind.contains(PURE(string)(ts2));
    // 1のほうが、ただのstringで、newtypeとして定義されたものならば、それを一回だけ展開してそれを比較する
    if ((ts1_expandable || ts2_expandable) && lvl < 2) {
        cout << "Expand" << endl;
        res = verify(ts1_expandable ? bind.at(PURE(string)(ts1)) : ts1,
                    ts2_expandable ? bind.at(PURE(string)(ts2)) : ts2,
                    bind, lvl+1);

        if (res)
            cout << "-> true" << endl;
        else
            cout << "-> false" << endl;
        return res;
    }

    if (MATCHS(TypeFn)(ts1) && MATCHS(TypeFn)(ts2)) {
        cout << "Function-wise" << endl;
        res = verify(PURES(TypeFn)(ts1)->to, PURES(TypeFn)(ts2)->to, bind, lvl) && res;
        res = PURES(TypeFn)(ts1)->from.size() == PURES(TypeFn)(ts2)->from.size() && res;
        for (int i = 0; i < PURES(TypeFn)(ts1)->from.size(); ++i) {
            res = verify(PURES(TypeFn)(ts1)->from[i], PURES(TypeFn)(ts2)->from[i], bind, lvl) && res;
        }
    } else if (MATCHS(TypeProduct)(ts1) && MATCHS(TypeProduct)(ts2)) {
        cout << "Product-wise" << endl;
        if (PURES(TypeProduct)(ts1)->products.size() != PURES(TypeProduct)(ts2)->products.size()) return false;
        for (int i = 0; i < PURES(TypeProduct)(ts1)->products.size(); ++i) {
            res = verify(PURES(TypeProduct)(ts1)->products[i], PURES(TypeProduct)(ts2)->products[i], bind, lvl) && res;
        }
    } else if (MATCH(string)(ts1) && MATCH(string)(ts2)) {
        cout << "string" << endl;
        res = PURE(string)(ts1) == PURE(string)(ts2);
    } else if (MATCH(SpecialValue)(ts1) && MATCH(SpecialValue)(ts2)) {
        cout << "SpecialValue" << endl;
        res = PURE(SpecialValue)(ts1).val == PURE(SpecialValue)(ts2).val;
    } else if (MATCHS(Parametered)(ts1) && MATCHS(Parametered)(ts2)) {
        cout << "Parametered" << endl;
        if (PURES(Parametered)(ts1)->params.size() != PURES(Parametered)(ts2)->params.size()) return false;
        res = PURES(Parametered)(ts1)->type == PURES(Parametered)(ts2)->type;
        for (int i = 0; i < PURES(Parametered)(ts1)->params.size(); i++) {
            res = verify(PURES(Parametered)(ts1)->params[i], PURES(Parametered)(ts2)->params[i], bind, lvl) && res;
        }
    } else if (MATCHS(TypeSum)(ts1)) {
        res = false;
        for (auto s1 : PURES(TypeSum)(ts1)->sums) {
            cout << "1 sum-wise" << PURES(TypeSum)(ts1)->sums.size() << endl;
            res = verify(ts2, s1, bind, lvl) || res;
        }
    } else if (MATCHS(TypeSum)(ts2)) {
        res = false;
        for (auto s2 : PURES(TypeSum)(ts2)->sums) {
            cout << "2 sum-wise" << endl;
            res = verify(ts1, s2, bind, lvl) || res;
        }
    } else {
        res = false;
    }

    if (res)
        cout << "-> true" << endl;
    else
        cout << "-> false" << endl;

    return res;
}
bool operator==(const TypeSignature &ts1, const TypeSignature &ts2) {
    bool res = equal(ts1, ts2);
    return res;
}

void deep_copy_from(TypeSignature &ts_dst, const TypeSignature &ts_src) {
    if (MATCHS(TypeFn)(ts_src)) {
        ts_dst = shared_ptr<TypeFn>(new TypeFn);
        deep_copy_from(PURES(TypeFn)(ts_dst)->to, PURES(TypeFn)(ts_src)->to);
        PURES(TypeFn)(ts_dst)->from.resize(PURES(TypeFn)(ts_src)->from.size());
        for (int i = 0; i < PURES(TypeFn)(ts_src)->from.size(); ++i) {
            deep_copy_from(PURES(TypeFn)(ts_dst)->from[i], PURES(TypeFn)(ts_src)->from[i]);
        }
    } else if (MATCHS(TypeSum)(ts_src)) {
        ts_dst = shared_ptr<TypeSum>(new TypeSum);
        for (auto &e : PURES(TypeSum)(ts_src)->sums) {
            TypeSignature tmp;
            deep_copy_from(tmp, e);
            PURES(TypeSum)(ts_dst)->sums.emplace_back(tmp);
        }
    } else if (MATCHS(TypeProduct)(ts_src)) {
        ts_dst = shared_ptr<TypeProduct>(new TypeProduct);
        PURES(TypeProduct)(ts_dst)->products.resize(PURES(TypeProduct)(ts_src)->products.size());
        PURES(TypeProduct)(ts_dst)->names.resize(PURES(TypeProduct)(ts_src)->products.size());
        for (int i = 0; i < PURES(TypeProduct)(ts_src)->products.size(); ++i) {
            deep_copy_from(PURES(TypeProduct)(ts_dst)->products[i], PURES(TypeProduct)(ts_src)->products[i]);
            PURES(TypeProduct)(ts_dst)->names[i] = PURES(TypeProduct)(ts_src)->names[i];
        }
    } else if (MATCH(string)(ts_src)) {
        ts_dst = PURE(string)(ts_src);
    } else if (MATCH(SpecialValue)(ts_src)) {
        ts_dst = PURE(SpecialValue)(ts_src);
    } else if (MATCHS(Parametered)(ts_src)) {
        ts_dst = shared_ptr<Parametered>(new Parametered);
        auto &param1 = PURES(Parametered)(ts_src);
        auto &param2 = PURES(Parametered)(ts_dst);
        param2->type =  param1->type;
        for (auto e : param1->params) {
            param2->params.emplace_back();
            deep_copy_from(param2->params.back(), e);
        }
    }
}

string to_string(const TypeSignature &typesig) {
    stringstream ss;
    if (MATCHS(TypeFn)(typesig)) {
        ss << "(";
        for (auto &e : PURES(TypeFn)(typesig)->from) {
            ss << to_string(e) << "->";
        }
        ss << to_string(PURES(TypeFn)(typesig)->to);
        ss << ")";
    } else if (MATCHS(TypeSum)(typesig)) {
        ss << "(";
        for (auto &e : PURES(TypeSum)(typesig)->sums) {
            ss << to_string(e) << "|";
        }
        ss << '\b' << ")";
    } else if (MATCHS(TypeProduct)(typesig)) {
        ss << "(";
        int i = 0;
        for (auto &e : PURES(TypeProduct)(typesig)->products) {
            if (PURES(TypeProduct)(typesig)->names.size()!=0) {
                ss << PURES(TypeProduct)(typesig)->names[i] << ":";
            }
            ss << to_string(e) << " ";
            i++;
        }
        ss << '\b' << ")";
    } else if (MATCH(string)(typesig)) {
        ss << PURE(string)(typesig);
    } else if (MATCH(SpecialValue)(typesig)) {
        ss << "*" << PURE(SpecialValue)(typesig).val;
    } else if (MATCHS(Parametered)(typesig)) {
        auto &param = PURES(Parametered)(typesig);
        ss << param->type << "<";
        for (auto e : param->params) {
            ss << to_string(e) << ",";
        }
        ss << ">";
    }
    return ss.str();
}

bool is_functional(const TypeSignature &ts) {
    return MATCHS(TypeFn)(ts);
}

bool arity(const TypeSignature &ts) {
    if (MATCHS(TypeFn)(ts)) return PURES(TypeFn)(ts)->from.size();
    else return 0;
}



// TypeSignature TypeSignature::outcome() const {
//     if (type_code->lit.val == "fn") {
//         return TypeSignature { type_code->args.back() };
//     } else return TypeSignature { type_code };
// }

// int TypeSignature::arity() const {
//     if (type_code->l) return TypeSignature(type_code->l->body).arity();
//     shared_ptr<Code> pivot = type_code;
//     int res = 0;
//     while (pivot->lit.val == "fn") {
//         res += pivot->args.size()-1;
//         pivot = pivot->args.back();
//     }
//     return res;
// }

// bool TypeSignature::functional() const { return arity() != 0; }

// bool TypeSignature::funcgen() const {
//     if (functional()) return outcome().functional();
//     return false;
// }

// void TypeSignature::apply(int n) {
//     assert(n <= arity());
//     shared_ptr<Code> pivot = type_code;
//     while (pivot->lit.val == "fn") {
//         if (n - pivot->args.size()-1 >= 0) {
//             n -= pivot->args.size()-1;
//             pivot = pivot->args.back();
//         } else break;
//     }
//     type_code = pivot;
//     for (; n <= 0; n--) type_code->args.pop_front();
// }

// void TypeSignature::wrap(const TypeSignature &typesig) {
//     if (functional()) {
//         type_code->args.push_front(shared_ptr<Code>(new Code));
//         type_code->args.front()->deep_copy_from(*typesig.type_code);
//     } else {
//         auto newtmp = shared_ptr<Code>(new Code);
//         newtmp->lit.val = "fn";
//         newtmp->args.push_front(shared_ptr<Code>(new Code));
//         newtmp->args.front()->deep_copy_from(*typesig.type_code);
//         newtmp->args.push_back(type_code);
//     }
// }

// void TypeSignature::normalize() {
//     // // fnはできるだけまとめる (fn A (fn B C)) -> (fn A B C)
//     // shared_ptr<Code> pivot = type_code;
//     // while (pivot->lit.val == "fn") {
//     //     for (const auto &e : pivot->args) {
//     //         Type
//     //             }
//     //     if (n - pivot->args.size()-1 >= 0) {
//     //         n -= pivot->args.size()-1;
//     //         pivot = pivot->args.back();
//     //     } else break;
//     // }
        
//     // // あとはor標準形にする (fn (or A B) (or C D) E) -> (or (fn A C E) (fn
//     // // (and (or A B) C)
        
// }

// void TypeSignature::normalized(shared_ptr<Code> c) const {
//     if (c->l) { return normalized(c->l->body); }
    
//     shared_ptr<Code> res = shared_ptr<Code>(new Code);
//     for (auto &e : c->args) {
//         normalized(e);
//     }
//     if (c->args.size() != 0) {
//         cout << TypeSignature(c->args.back()).to_string() << endl;
//         cout << TypeSignature(c->args.back()).arity() << endl;
//     }
//     if (c->lit.val == "fn" && TypeSignature(c->args.back()).functional()) {
//         // (fn A (fn B C)) -> (fn A B C)
//         vector<shared_ptr<Code>> tmp;
//         copy(c->l->body->args.back()->args.begin(), c->l->body->args.back()->args.end(), back_inserter(tmp));
//         c->l->body->args.pop_back();
//         for (const auto &e : tmp) {
//             cout << 'a' << endl;
//             c->l->body->args.push_back(e);
//         }
            
//     }//  else if (c->lit.val == "or") {
//     // } else {
//     // }
// }

// string TypeSignature::to_string() const { return to_string(type_code); }

// string TypeSignature::to_string(const shared_ptr<Code> c) const {
//     stringstream ss;
//     if (c->lit.val == "fn") {
//         ss << "(";
//         for (auto &e : c->args) {
//             ss << to_string(e) << "->";
//         }
//         ss << '\b' << '\b';
//         ss << ")";
//     } else if (c->lit.val == "or") {
//         ss << "(";
//         for (auto &e : c->args) {
//             ss << to_string(e) << "|";
//         }
//         ss << '\b' << ")";
//     } else if (c->lit.val == "and") {
//         ss << "(";
//         for (auto &e : c->args) {
//             ss << to_string(e) << " ";
//         }
//         ss << '\b' << ")";
//     } else if (c->l) {
//         ss << to_string(c->l->body);
//     } else {
//         ss << c->lit.val;
//     }
//     return ss.str();
// }



// struct TypeSignature {
//     deque<Type> from;
//     Type to;

//     TypeSignature() {}
//     ~TypeSignature() {}
//     TypeSignature(deque<Type> a, Type b) : from(a), to(b) {} //TODO
//     TypeSignature(Type a) {
//         from = {};
//         to = a;
//     }
//     TypeSignature(deque<Type> a) {
//         to = a.back();
//         a.pop_back();
//         from = a;
//     }

//     int arity() const { return from.size(); }
//     int functional() const { return arity() != 0; }
//     int funcgen() const {
//         if (holds_alternative<shared_ptr<TypeSignature>>(to)) {
//             return get<shared_ptr<TypeSignature>>(to)->functional();
//         }
//         return false;
//     }
//     TypeSignature outcome() const { return TypeSignature { {}, to }; }
//     void apply(int n) {
//         assert(n <= int(from.size()));

//         for (int i = 0; i < n; i++) from.pop_front();

//         // None -> (a -> a) みたいになっている場合は a -> a としたい
//         // apply はこのようになる可能性のある操作
//         while (from.size() == 0) {
//             if (holds_alternative<shared_ptr<TypeSignature>>(to)) {
//                 auto typesig = get<shared_ptr<TypeSignature>>(to);
//                 from = typesig->from;
//                 to = typesig->to;
//             } else break;
//         }
//     }
//     void wrap(const TypeSignature &typesig) {
//         if (typesig.from.size() == 0) {
//             from.push_front(typesig.to); // TODO
//         } else {
//             from.push_front(shared_ptr<TypeSignature>(new TypeSignature));
//             get<shared_ptr<TypeSignature>>(from.front())->deep_copy_from(typesig);
//         }
//     }
//     void normalize() {
//         TypeSignature res = *this;
//         while (holds_alternative<shared_ptr<TypeSignature>>(res.to)) {
//             auto typesig = get<shared_ptr<TypeSignature>>(res.to);
//             for (int i = 0; i < typesig->from.size(); i++) {
//                 res.from.push_back(typesig->from[i]);
//             }
//             res.to = typesig->to;
//         }

//         for (int i = 0; i < res.from.size(); i++) {
//             Type res1 = from[i];
//             if (holds_alternative<shared_ptr<TypeSignature>>(res1)) {
//                 shared_ptr<TypeSignature> e = get<shared_ptr<TypeSignature>>(res1);
//                 if (holds_alternative<string>(e->to)) {
//                     res1 = e->to;
//                 }
//             }
//             from[i] = res1;
//         }
//     }
//     // NOTE: deep copyはしないのでこれを変更しないように
//     const TypeSignature normalized() const {
//         TypeSignature res = *this;
//         while (holds_alternative<shared_ptr<TypeSignature>>(res.to)) {
//             auto typesig = get<shared_ptr<TypeSignature>>(res.to);
//             for (int i = 0; i < typesig->from.size(); i++) {
//                 res.from.push_back(typesig->from[i]);
//             }
//             res.to = typesig->to;
//         }
//         return res;
//     }
//     TypeSignature arg(int n) const {
//         assert(n <= int(from.size()-1));
//         return TypeSignature { {} , {from[n]} };
//     }
//     string to_string() const {
//         string res = "";
//         for (auto f : from) {
//             if (holds_alternative<string>(f)) { res += get<string>(f) + "-"; }
//             else { res += "(" + get<shared_ptr<TypeSignature>>(f)->to_string() + ")-"; }
//         }
//         if (from.size() != 0) res += ">";
//         if (holds_alternative<string>(to)) { res += get<string>(to); }
//         else {
//             res += "("+get<shared_ptr<TypeSignature>>(to)->to_string()+")";
//         }
//         return res;
//     }
//     void deep_copy_from(const TypeSignature &other) {
//         if (holds_alternative<string>(other.to)) {
//             to = other.to;
//         }
//         else {
//             to = shared_ptr<TypeSignature>(new TypeSignature);
//             get<shared_ptr<TypeSignature>>(to)->deep_copy_from(*get<shared_ptr<TypeSignature>>(other.to));
//         }

//         from = {};
//         for (auto f : other.from) {
//             if (holds_alternative<string>(f)) {
//                 from.push_back(f);
//             } else {
//                 from.push_back(shared_ptr<TypeSignature>(new TypeSignature));
//                 get<shared_ptr<TypeSignature>>(from.back())->deep_copy_from(*get<shared_ptr<TypeSignature>>(f));
//             }
//         }
//     }
// };
// inline bool operator==(const TypeSignature &lhs, const TypeSignature &rhs) {
//     return (lhs.from == rhs.from) && (lhs.to == rhs.to);
// }
// inline bool operator!=(const TypeSignature &lhs, const TypeSignature &rhs) {
//     return ! (lhs == rhs);
// }
