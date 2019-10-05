/* ========================================================================
   $File: transpile.cpp $
   $Date: Sep 09 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"
#include "notation.h"

const char * c_head =
    "#include <stdbool.h>\n"
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "#include <math.h>\n"
    "\n"
    "#define STACK_SIZE 1000\n"
    "#define VARIABLE_NUM 10\n"
    "\n"
    "typedef struct {\n"
    "    // 32-bit\n"
    "    unsigned long length;\n"
    "    char *address;\n"
    "} DataPointer;\n"
    "\n"
    "static char *stack[STACK_SIZE];\n"
    "static char **sp;\n"
    "static DataPointer dptrs[VARIABLE_NUM];\n"
    "\n"
    "#define digit(n) ((n) == 0 ? 1 : ((int) log10(abs((n)))+1+((n) ? 1 : 0)))\n"
    "#define push(sp, n) (*((sp)++) = (n))\n"
    "#define top(sp) *(sp-1)\n"
    "#define mpush(sp, n) (*((sp)++) = malloc(sizeof(char) * (strlen(n) + 1))); strcpy(top(sp), n)\n"
    "#define mpop(sp) free(*(((sp)--)-1)); *sp = 0\n"
    "#define pop(sp) *(sp-1) = 0; sp--;\n"
    "#define bind(sp, var) dptrs[var] = (DataPointer) { .length = strlen(top(sp)), .address = /*top(sp)*/(char *) calloc(sizeof(char), strlen(top(sp))+1) }; strcpy(dptrs[var].address, top(sp)); mpop(sp); \n"
    "#define ref(var) dptrs[var].address\n"
    "\n"
    "void free_all() {\n"
    "	for (int i = 0; i < VARIABLE_NUM; i++) {\n"
    "		if (dptrs[i].address) free(dptrs[i].address);\n"
    "	}\n"
    "}\n"
    "\n"
    "\n"
    "\n"
    "// built-in functions\n"
    "inline void concat() {\n"
    "    char *a = *(sp-1);\n"
    "    char *b = *(sp-2);\n"
    "\n"
    "    char *tmp = (char *) calloc(sizeof(char), strlen(a) + strlen(b) + 1);\n"
    "    sprintf(tmp, \"%s%s\", a, b);\n"
    "\n"
    "    mpop(sp);\n"
    "    mpop(sp);\n"
    "    push(sp, tmp);\n"
    "}\n"
    "\n"
    "\n"
    "inline void add() {\n"
    "    int a = atoi(top(sp)); mpop(sp);\n"
    "    int b = atoi(top(sp)); mpop(sp);\n"
    "\n"
    "    char *tmp = (char *) calloc(sizeof(char), (digit(a+b) + 1));\n"
    "    sprintf(tmp, \"%d\", a + b);\n"
    "    push(sp, tmp);\n"
    "}\n"
    "\n"
    "inline void negate() {\n"
    "    int a = atoi(top(sp)); mpop(sp);\n"
    "\n"
    "    char *tmp = (char *) calloc(sizeof(char), (digit(-a) + 1));\n"
    "    sprintf(tmp, \"%d\", -a);\n"
    "    push(sp, tmp);\n"
    "\n"
    "}\n"
    "\n"
    "// (|sub| sub 3 5) (|a b| add a (negate b))\n"
    "\n"
    "int main() {\n"
    "    sp = stack;\n";

const char * c_tail =
    "        // ここでdptrも初期化されることに注意\n"
    "    printf(\"STACK: \\n\");\n"
    "    while (sp != stack) {\n"
    "        printf(\"%s\\n\", top(sp));\n"
    "        mpop(sp);\n"
    "    }\n"
    "\n"
    "    free_all();\n"
    "\n"
    "    return 0;\n"
    "}";

void assign_varnum(const shared_ptr<Code> c, map<string, int> &evo) {
    if (c->l) {
        for (auto n : c->l->argnames) {
            if (!CONTAINS(evo, n)) {
                evo[n] = evo.size();
            }
        }
        assign_varnum(c->l->body, evo);
    }

    for (auto e : c->args) {
        assign_varnum(e, evo);
    }
}

void rename_variables(const shared_ptr<Code> c,
                      map<string, int> varused_counter = {})
{

    for (auto e : c->args) {
        rename_variables(e, varused_counter);
    }

    if (c->l) {
        for (auto &n : c->l->argnames) {
            varused_counter[n]++;
            if (varused_counter[n] != 1)
                n = /*"____renamed____" + */ n + to_string(varused_counter[n]);
        }
        rename_variables(c->l->body, varused_counter);
    } else {
        if (!is_literal(c->lit.val)) {
            if (varused_counter[c->lit.val] >= 2)
                c->lit.val = /*"____renamed____" + */c->lit.val + to_string(varused_counter[c->lit.val]);
        }
    }
}


/*
  TODO: 標準ライブラリ
  全ての論理関数を作ること

  このままだと、ヒープを全く使わない
 */
/*
  NOTE: 
  関数のスコープの問題がある

  何かというと、
  (|a b| (|res| (|c| res)) (|...| ... c ...) )
  みたいなコードが合った時に、普通に考えると、res関数内にはa,bの情報しか行かないけど、
  コード生成アルゴリズム的、インタプリタ的にもこれを保存しておくのは大変な仕事なので、実際このコードでは
  cのじょうほうも行ってしまう。

  この問題は、実行側では解決せずに、コンパイル時の静解析でやるべき。（できるだけ実行時のコストは落とす）


  同じようなやつで,
  (|res| res 3 (res 4 5)) (|a b| ...)
  こういうのがあった時、左の方の関数の中でa, bが使えてしまう問題がある。これもコンパイル時の正解積でやるべきなの...?
  でもスタックでやってるから...
 */
// pair<string, string> c_(shared_ptr<Code> c,
//                         map<string, int> vartable,
//                         bool strict,
//                         stack<shared_ptr<Code>> argstack = {},
//                         map<int, shared_ptr<Code>> bind = {}) {
//     string res = "";
//     string functions = "";
//     stack<shared_ptr<Code>> outer_argstack = argstack;
    
//     cout << "#############################################################" << endl;
//     cout << *c << endl << endl;
//     cout << "FUNCTIONS: " << functions << endl;
//     cout << "MAIN: " << res << endl;
//     cout << "STACK: " <<endl;
//     auto tmp = argstack;
//     for (int i = 0; i < argstack.size(); i++) {
//         cout << *tmp.top() << endl << endl;
//         tmp.pop();
//     }
//     cout << "BIND: " <<endl;
//     for (auto tmp : vartable) {
        
//         if (CONTAINS(bind, tmp.second))
//             cout << tmp.first << " => " << *bind[tmp.second] << endl << endl;
//     }
//     cout << endl << endl << endl;
    
//     for (int i = c->args.size()-1; i >= 0; --i) {
//         argstack.push(c->args[i]);
//         // res += c_(c->args[i], vartable);
//         // stack_length++;
//     }

//     if (c->l) {
//         int argi = 0;
//         for (auto n : c->l->argnames) {
// //            vartable[n] = vartable.size();
//             if (argstack.size() == 0) {
//                 vartable[n] = argi;
//             } else {
// //                res += "bind(sp, " + to_string(vartable[n]) + string(");\n");
// //                cout << n << " => " << *argstack.top() << endl;;

//                 if (strict) {
//                     cout << "一般関数引数< " << n << endl;
//                     auto tmp = c_(argstack.top(), vartable, strict, {}, bind);
//                     functions += "v" + to_string(vartable[n]) + ": \n";
//                     functions += tmp.first;
//                     functions += "goto _v" + to_string(vartable[n]) + ";\n";
//                     cout << "v" + to_string(vartable[n]) + ": \n";
//                     cout << tmp.first;
//                     cout << "goto _v" + to_string(vartable[n]) + ";\n";
//                     cout << "> " << n << endl;
//                     bind[vartable[n]] = argstack.top();
//                 } else {
//                     bind[vartable[n]] = argstack.top();
//                 }
//                 argstack.pop();
//             }
//             argi++;
//         }

//         auto tmp = c_(c->l->body, vartable, strict, argstack, bind);
//         return make_pair(res + tmp.first, functions + tmp.second);
//     } else {
//         // cout << c->lit.val << endl;
//         // cout << c->args.size() << endl;
        
//         // literal or
//         if (c->lit.val == "negate") {
//             cout << "関数適用 negate\n";
//             res += c_(argstack.top(), vartable, strict, {}, bind).first; argstack.pop();
//             res += "negate();\n";
//         } else if (c->lit.val == "add") {
//             cout << "関数適用 add\n";
//             res += c_(argstack.top(), vartable, strict, {}, bind).first; argstack.pop();
//             res += c_(argstack.top(), vartable, strict, {}, bind).first; argstack.pop();
//             res += "add();\n";
//         } else if (c->lit.val == "concat") {
//             cout << "関数適用 concat\n";
//             res += c_(argstack.top(), vartable, strict, {}, bind).first; argstack.pop();
//             res += c_(argstack.top(), vartable, strict, {}, bind).first; argstack.pop();
//             res += "concat();\n";
//         } else if (c->lit.val == "fuse") {
//             cout << "関数適用 fuse\n";
//             auto f1 = argstack.top(); argstack.pop();
//             auto f2 = argstack.top(); argstack.pop();
//             stack<shared_ptr<Code>> tmp_argstack1 = argstack;
//             stack<shared_ptr<Code>> tmp_argstack2 = argstack;

//             // 引数を先に計算
//             for (int i = 0; i < f1->l->argnames.size(); ++i)
//                 res += c_(argstack.top(), vartable, strict, {}, bind).first; argstack.pop();

//             res += "if (";
//             for (int i = 0; i < f1->l->argnames.size(); ++i) {
//                 if (is_literal(f1->l->argnames[i])) {
//                     res += "strcmp(*(sp-" + to_string(1+i) + "), \""+f1->l->argnames[i]+"\") && ";
//                 }
//             }
//             res += "true) {\n";
//             res += c_(make_shared<Code>(Code{f1->l, ""}), vartable, strict, tmp_argstack1, bind).first; // rf + 何か
//             res += "}\n";

//             res += "else if (";
//             for (int i = 0; i < f2->l->argnames.size(); ++i) {
//                 if (is_literal(f2->l->argnames[i])) {
//                     res += "strcmp(*(sp-" + to_string(1+i) + "), \""+f2->l->argnames[i]+"\") && ";
//                 }
//             }
//             res += "true) {\n";
//             res += c_(make_shared<Code>(Code{f2->l, ""}), vartable, strict, tmp_argstack2, bind).first; // rf + 何か
//             res += "\n}\n";

//         } else if (is_literal(c->lit.val) || (!is_literal(c->lit.val) && !CONTAINS(vartable, c->lit.val))) {
//             // リテラル
//             cout << "リテラル" + c->lit.val << endl;
//             res += "mpush(sp, \"" + c->lit.val + "\");\n";
//         } else {
//             cout << "関数適用 " + c->lit.val + "\n";

//             if (strict) {
//                 res += "goto v" + to_string(vartable[c->lit.val]) + ";\n";
//                 res += "_v" + to_string(vartable[c->lit.val]) + ": \n";
//             } else {
//                 res += c_(bind[vartable[c->lit.val]], vartable, strict, argstack, bind).first; // rf + 何か
//             }
//             cout << "]";
//         }
//     }


//     return make_pair(res, functions);
// }

pair<string, string> c_s(shared_ptr<Code> c,
                         map<string, int> vartable,
                         stack<shared_ptr<Code>> argstack = {},
                         map<int, shared_ptr<Code>> bind = {},
                         map<string, int> varused_counter = {}) {
    string res = "";
    string functions = "";
    stack<shared_ptr<Code>> outer_argstack = argstack;
    
    cout << "#############################################################" << endl;
    cout << *c << endl << endl;
    cout << "BIND: " <<endl;
    for (auto tmp : vartable) {
        
        if (CONTAINS(bind, tmp.second))
            cout << tmp.first << " => " << *bind[tmp.second] << endl << endl;
    }
    
    cout << endl << endl << endl;
    
    for (int i = c->args.size()-1; i >= 0; --i) {
//        if (is_literal(c->args[i]->lit.val)) continue;
        argstack.push(c->args[i]);
        // res += c_(c->args[i], vartable);
        // stack_length++;
    }

        cout << "STACK: " <<endl;
    auto tmp = argstack;
    for (int i = 0; i < argstack.size(); i++) {
        cout << *tmp.top() << endl << endl;
        tmp.pop();
    }


    if (c->l) {
        res += "{\n";
        
        for (auto n : c->l->argnames) {
            if (is_literal(n)) {
                argstack.pop();
                continue;
            }

            if (argstack.size() == 0) {
            } else {
                if (!CONTAINS(varused_counter, n)) varused_counter[n] = 1;
                else varused_counter[n]++;

                cout << "引数: " + n /*+ to_string(varused_counter[n])*/ << endl;

                auto tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);

                res += tmp.second;
                res += "inline void " + n /*+ to_string(varused_counter[n])*/ + "() {\n";
                res += tmp.first;
                res += "}\n";
            
                bind[vartable[n]] = argstack.top();
                argstack.pop();
            }
        }

        auto tmp = c_s(c->l->body, vartable, argstack, bind, varused_counter);

        cout << "!!!" << endl;
        cout << res  + tmp.first << "\n\n" << res + tmp.second;

        res += tmp.first;
        res += tmp.second;

        res += "}\n";
        
        return make_pair(res, functions);
    } else {
        // cout << c->lit.val << endl;
        // cout << c->args.size() << endl;
        
        // literal or
        if (c->lit.val == "negate") {
            cout << "関数適用 negate\n";
            auto tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
            res += tmp.first; res += tmp.second; argstack.pop();
            res += "negate();\n";
        } else if (c->lit.val == "add") {
            cout << "関数適用 add\n";
            auto tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
            res += tmp.first; res += tmp.second; argstack.pop();
            
            tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
            res += tmp.first; res += tmp.second; argstack.pop();
            res += "add();\n";
        } else if (c->lit.val == "concat") {
            cout << "関数適用 concat\n";
            auto tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
            res += tmp.first; res += tmp.second; argstack.pop();
            
            tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
            res += tmp.first; res += tmp.second; argstack.pop();
            res += "concat();\n";
        } else if (c->lit.val == "fuse") {
            cout << "関数適用 fuse\n";
            auto f1 = argstack.top(); argstack.pop();
            auto f2 = argstack.top(); argstack.pop();
            auto tmp_argstack1 = argstack;
            auto tmp_argstack2 = argstack;

            // 引数を先に出しておく
            for (int i = 0; i < f1->l->argnames.size(); ++i) {
                auto tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
                res += tmp.first; res += tmp.second;
                argstack.pop();
            }

            // 先に出した引数を見て分岐
            res += "if (";
            for (int i = 0; i < f1->l->argnames.size(); ++i) {
                if (is_literal(f1->l->argnames[i])) {
                    res += "strcmp(*(sp-" + to_string(f1->l->argnames.size()-i) + "), \""+f1->l->argnames[i]+"\") && ";
                }
            }

            res += "true) {\n";
            auto tmp = c_s(f1, vartable, tmp_argstack1, bind, varused_counter);
            res += tmp.first; res += tmp.second;
            res += "}\n";

            res += "else if (";
            for (int i = 0; i < f2->l->argnames.size(); ++i) {
                if (is_literal(f2->l->argnames[i])) {
                    res += "strcmp(*(sp-" + to_string(f2->l->argnames.size()-i) + "), \""+f2->l->argnames[i]+"\") && ";
                }
            }
            res += "true) {\n";
            tmp = c_s(f2, vartable, tmp_argstack2, bind, varused_counter);
            res += tmp.first; res += tmp.second;
            res += "\n}\n";

            for (int i = 0; i < f1->l->argnames.size(); ++i) {
                res += "mpop(sp);\n";
            }
        } else if (is_literal(c->lit.val) || (!is_literal(c->lit.val) && !CONTAINS(vartable, c->lit.val))) {
            // リテラル
            cout << "リテラル" + c->lit.val << endl;
            res += "mpush(sp, \"" + c->lit.val + "\");\n";
        } else {
            cout << "関数適用 " + c->lit.val + "\n";

            // argstack を全てぶちまける
            while (!argstack.empty()) {
                auto tmp = c_s(argstack.top(), vartable, {}, bind, varused_counter);
                res += tmp.first; res += tmp.second; argstack.pop();
            }

            // if (varused_counter[c->lit.val] == 0) {
            //     int dist = distance(unbinded.begin(), find(unbinded.begin(), unbinded.end(), c->lit.val + to_string(varused_counter[c->lit.val])));
            //     res += "mpush(sp, *(sp-" + to_string(dist+1) + "));\n";
                
            // }
            // functionに入れる
            res += c->lit.val /*+ to_string(varused_counter[c->lit.val])*/ + "();\n";
            
//            res += c_(bind[vartable[c->lit.val]], vartable, argstack, bind); // rf + 何か
        }
    }


//    cout << "] (" << res << ", " << functions << ")" << endl;
    return make_pair(res, functions);
}



string asm_(shared_ptr<Code> c) {
}

string transpile(shared_ptr<Code> c, const string dest) {
    
    if (dest == "c") {
        map<string, int> vartable;
        assign_varnum(c, vartable);

        for (auto a : vartable) {
            cout << a.first << " " << a.second << endl;
        }

        rename_variables(c, {});

        auto tmp = c_s(c, vartable);
        return c_head + tmp.first + /*tmp.second + */c_tail;
    } else if (dest == "asm") {
        return asm_(c);
    }

    return "";
}
