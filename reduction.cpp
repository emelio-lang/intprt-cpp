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
            return make_pair(code, rf); // NOTE: ここのliteralを入れておくことでbodyが無いことを示せる
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
