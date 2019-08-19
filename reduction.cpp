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

pair<Code, ReductionFlow> reduction(Code code, ReductionFlow rf) {

    if (!code.l) {
        if (is_literal(code.lit.val)) return make_pair(code,rf);
        else {
            if (CONTAINS(rf.bind, code.lit.val)) {
                Code res = *rf.bind[code.lit.val];
                while (!res.l) {
                    if (is_literal(res.lit.val)) return make_pair(res, rf);
                    res = *rf.bind[res.lit.val];
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

    for (int i = code.args.size()-1; i >= 0; i--)
        rf.argstack.push(&code.args[i]);

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
        rf.bind[argname] = arg;
    }

    if (code.lit.val == "add") {
        Code a1 = reduction(*rf.bind["a1"],rf).first;
        Code a2 = reduction(*rf.bind["a2"],rf).first;
        pair<int,bool> tmp1 = read_int_litcode(a1);
        pair<int,bool> tmp2 = read_int_litcode(a2);
            
        if (tmp1.second && tmp2.second) 
            return make_pair(make_int_litcode(tmp1.first + tmp2.first), rf);
        else
            return make_pair(Code {&add, "add", code.args}, rf); // NOTE: ここのliteralを入れておくことでbodyが無いことを示せる
    } else if (code.lit.val == "negate") {
        Code a1 = reduction(*rf.bind["a1"],rf).first;
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


Code reduction(Code code) {
    ReductionFlow rf = {};
    return reduction(code, rf).first;
}


// Code F_reduction(Code code, ReductionFlow rf) {
//     if (!code.l) return code;

//     ReductionFlow oldrf = rf;
//     for (Code arg : code.args) {
//         reduction(arg, oldrf);
//     }
// }
