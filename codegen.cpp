/* ========================================================================
   $File: codegen.cpp $
   $Date: Nov 11 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"
#include "notation.h"
#include "codegen.h"

#include <sstream>

const set<string> builtin_functions = {
    "add", "sub", "mul", "div", "negate", "concat", "fuse"
};

const map<string, unsigned> bf2arity = {
    { "negate", 1 },
    { "add", 2 },
    { "sub", 2 },
    { "mul", 2 },
    { "div", 2 },
    { "concat", 2 },
};

const map<string, TypeSignature> bf2typesig = {
    { "negate", TypeSignature { {"int"}, {"int"} }},
    { "add", TypeSignature { {"int", "int"}, {"int"} }},
    { "sub", TypeSignature { {"int", "int"}, {"int"} }},
    { "mul", TypeSignature { {"int", "int"}, {"int"} }},
    { "div", TypeSignature { {"int", "int"}, {"int"} }},
    { "concat", TypeSignature { {"string", "string"}, {"string"} }},
};

#define CODEGEN_DELIMITER " "

inline void outprg(string &res, const string &body) { res += body + (CODEGEN_DELIMITER); }
inline void outprg(string &res, string &env, const pair<string,string> &bodyenv) { res += bodyenv.first + (CODEGEN_DELIMITER); env += bodyenv.second + (CODEGEN_DELIMITER); }

inline void outprg(vector<string> &res, const string &body) {
    res.push_back(body);
}
inline void outroot(StackLanguage &dist, const StackLanguage &src) {
    copy(src.root.begin(), src.root.end(), back_inserter(dist.root));
    copy(src.env.begin(), src.env.end(), back_inserter(dist.env));
}
inline void outenv(StackLanguage &dist, const StackLanguage &src) {
    copy(src.root.begin(), src.root.end(), back_inserter(dist.env));
    copy(src.env.begin(), src.env.end(), back_inserter(dist.env));
}

string StaticProgram = "";
unsigned function_call_counter = 0;
unsigned conditional_counter = 0;

Guard get_guard(const vector<shared_ptr<Code>> &args) {
    Guard res;
    for (auto arg : args) {
        if (is_literal(arg->l->argnames[0])) {
            res.finites.push_back(make_pair(arg->l->argnames[0], arg->l->body));
        } else {
            res.countable = arg;
        }
    }
    return res;
}

GuardType get_guard_type(const vector<shared_ptr<Code>> &args) {
    for (auto arg : args) {
        if (!is_literal(arg->l->argnames[0])) {
            return GTYPE_COUNTABLE_FINITE;
        }
    }
    return GTYPE_FINITE;
}

/*
  cgen(add e1 ...) =
  cgen(e1)
  ...
  pop $1
  pop $2
  add $1, $1, $2
  push $1


*/

/* TODO:
   ・配列データ
   ・bindされない引数の扱い
 */

void rename_variables(const shared_ptr<Code> c)
{
    map<string, int> varused_counter = {};

    function<void(const shared_ptr<Code>)> _rename_variables =
        [&](const shared_ptr<Code> c) {
            for (auto e : c->args) {
                _rename_variables(e);
            }

            if (c->l) {
                for (auto &n : c->l->argnames) {
                    // 数字なら飛ばす　1が何回もfuseに現れるからって12, 13とかにされても困るよね
                    if (is_number(n)) continue;

                    varused_counter[n]++;
                    if (varused_counter[n] != 1)
                        n = /*"____renamed____" + */ n + to_string(varused_counter[n]);
                }
                _rename_variables(c->l->body);
            } else {
                if (!is_literal(c->lit.val)) {
                    if (varused_counter[c->lit.val] >= 2)
                        c->lit.val = /*"____renamed____" + */c->lit.val + to_string(varused_counter[c->lit.val]);
                }
            }

        };

    _rename_variables(c);
}

// template<typename T>
// void SCOUT(const stack<T> &s) {
//     auto tmp = s;
//     while (!tmp.empty()) {
//         cout << tmp.top() << " ";
//         tmp.pop();
//     }
//     cout << endl;
// }

template<typename T>
void SCOUT(deque<shared_ptr<T>> s) {
    while (!s.empty()) {
        cout << *s.back() << endl;
        s.pop_back();
    }
    cout << endl;
}

void set_arity::operator () (const shared_ptr<Code> c) {
    if (c->l) {
        c->arity += c->l->argnames.size();

        for (int i = c->args.size()-1; i >= 0; i--) {
            (set_arity(&bind))(c->args[i]);
            argstack.push(c->args[i]);
        }

        int i = 0;
        for (auto argname : c->l->argnames) {
//            cout << argname << endl;
            if (argstack.empty()) {
                // 引数情報から推論できない場合でも、種の指定があればそれとする。
                // 指定もなければbindに入れないことでマークしておく（下の方の処理でないと推論の情報があった時つじつまを合わせる）
                if (c->l->argarities[i] != -1)
                    bind[argname] = c->l->argarities[i];
                continue;
            }
//            cout << *argstack.top() << endl;
            bind[argname] = argstack.top()->arity;
            // 種の指定があれば、整合性をチェック
            if (c->l->argarities[i] != -1)
                ASSERT(bind[argname] == c->l->argarities[i],
                       ("引数"+argname+"の種が合致しません. (expected:" + to_string(c->l->argarities[i]) +
                        " inferred:"+to_string(bind[argname])+")"));
            argstack.pop();
            i++;
        }

        this->operator()(c->l->body);
        c->arity += c->l->body->arity;

    }
    else if (c->lit.val == "fuse") {
        for (auto a : c->args) (set_arity(&bind))(a);

        c->arity = c->args[0]->arity + c->args.size() /*for adjust*/; // TODO?
    }
    else {
        if (!c->l && builtin_functions.contains(c->lit.val)) {
            c->arity += bf2arity.at(c->lit.val);
        }
        else { // what..
            // 不明な関数は分からないのでarityは引数の数で値を返す。つまり無条件で0になるようにする
            try {
//                cout << c->lit.val << " " << bind.at(c->lit.val) << endl;
                c->arity += bind.at(c->lit.val);
            } catch (exception &e) {
                c->arity += c->args.size();
            }
        }
            
        for (auto a : c->args) (set_arity(&bind))(a);
    }

//    cout << "a" << endl;

    c->arity -= c->args.size();
}

// プログラム中で型指定されているlambdaしか情報がないので、その情報などをもとに全てのlambda, codeに妥当な型を当てはめます
// (|  |  body ) O O とあった時、(|1| 3  ) 2 の順番で処理していきます。
// まず、1で指定された型の情報を信用してbindに保存し、
// ２その情報を持って順番に引数を処理して、最終的な型が1と合致することを確かめます。
// ３そしてbodyを処理して
// ４その全体のCodeの型を3で得た型に引数の個数だけ適用した型として決定します

// fn O O とあった時、fnの型はすでに分かっていないとエラーにする. Oから
void set_type::operator () (const shared_ptr<Code> c) {
    if (c->l) {
        // 1
        int i = 0;
        for (auto argname : c->l->argnames) {
            bind[argname] = c->l->argtypes[i];
            i++;
        }

        // 3
        this->operator()(c->l->body);

        // 2
        for (int i = c->args.size()-1; i >= 0; i--) {
            (set_type(&bind))(c->args[i]);
            // argstack.push(c->args[i]);
        }

        // 2 合致することを確かめます
        for (int i = 0; i < min(c->args.size(),c->l->argnames.size()); i++) {
            auto argname = c->l->argnames[i];
            ASSERT(bind[argname].normalized() == c->args[i]->type.normalized(),
                   ("引数"+argname+"の型が合致しません. (expected:" + bind[argname].normalized().to_string() +
                    " inferred:"+c->args[i]->type.normalized().to_string()+")"));
        }


        // 4
        c->l->type = c->l->body->type;
        c->type = c->l->body->type;
        for (int i = 0; i < c->l->argtypes.size(); ++i) {
            c->l->type.wrap(c->l->argtypes[i]);
            c->type.wrap(c->l->argtypes[i]);
        }
        c->type.apply(c->args.size());
    }
    else if (c->lit.val == "fuse") {
        // TODO
    }
    else if (c->lit.val == "type") {
        TypeSignature tmp;
        tmp.to = c->args[0]->lit.val;
        tmp.normalize();
        int i = 0;
        for (auto typ : c->args[1]->l->argtypes) {
            // NOTE: c->args[1]->lへの参照を使用しています！！
            tmp.from.push_back(make_shared<TypeSignature>(typ));

            TypeSignature memfn;
            memfn.from.push_back(tmp.to);
            memfn.to = typ.to;
            memfn.normalize();
            bind[c->args[1]->l->argnames[i]] = memfn;
            i++;
        }
        bind[c->args[0]->lit.val] = tmp;
        
//        type_constructors[c->args[0]->lit.val] = c->args[1]->l;
        this->operator()(c->args[2]);
    } else if (builtin_functions.contains(c->lit.val)) {
        for (auto a : c->args) (set_type(&bind))(a);

        c->type.deep_copy_from(bf2typesig.at(c->lit.val));
        c->type.apply(c->args.size());

        int i = 0;
        for (auto a : c->args) {
            ASSERT(bf2typesig.at(c->lit.val).arg(i).normalized() == c->args[i]->type.normalized(),
                   "'"+c->lit.val+"'の"+to_string(i)+"番目の引数の型が合致しません. (expected:" + bf2typesig.at(c->lit.val).to_string() +
                   " inferred:"+c->args[i]->type.to_string()+")");
        }
    // } else if (type_constructors.contains(c->lit.val)) {
    //     for (auto a : c->args) (set_type(&bind))(a);

    //     c->type.deep_copy_from(bind[c->lit.val]);
    //     c->type.apply(c->args.size());

    //     int i = 0;
    //     for (auto a : c->args) {
    //         ASSERT(bind[c->lit.val].arg(i).normalized() == c->args[i]->type.normalized(),
    //                "'"+c->lit.val+"'の"+to_string(i)+"番目の引数の型が合致しません. (expected:" + bind[c->lit.val].to_string() +
    //                " inferred:"+c->args[i]->type.to_string()+")");
    //     }
    } else if (is_literal(c->lit.val)) {
        if (is_number(c->lit.val)) {
            c->type = TypeSignature("int");
        }
    } else {
        const auto fnname = c->lit.val;
        for (auto a : c->args) (set_type(&bind))(a);

        // もし、関数の型が分かっていないならとりまエラー
        ASSERT(bind.contains(fnname), "'"+fnname+"' の型が不明です");

        c->type.deep_copy_from(bind[fnname]);
        c->type.apply(c->args.size());

        int i = 0;
        for (auto a : c->args) {
            ASSERT(bind[fnname].arg(i).normalized() == c->args[i]->type.normalized(),
                   "'"+c->lit.val+"'の"+to_string(i)+"番目の引数の型が合致しません. (expected:" + bind[fnname].to_string() +
                   " inferred:"+c->args[i]->type.to_string()+")");
        }
    }
    cout << * c << endl;

//    cout << "a" << endl;
}

#include "codegen1.cpp"
#include "codegen2.cpp"
#include "codegen3.cpp"
#include "codegen4.cpp"
#include "codegen5.cpp"
#include "codegen6.cpp"
#include "ocamlgen.cpp"

vector<string> split_instr(string instr) {
    vector<string> res;
    replace(instr.begin(), instr.end(), ',', ' ');
    istringstream iss(instr);
    string s;
    while (iss >> s) {
        res.push_back(s);
    }
    return res;
}

string join_instr(vector<string> instrs) {
    if (instrs.size() == 0) return "";
    
    string res;
    res += instrs[0] + " ";
    for (int i = 1; i < instrs.size(); i++) {
        res += instrs[i] + ",";
    }
    res.pop_back();
    return res;
}

string fasm_ins(string ins) {
    if (is_literal(ins)) {
        return
            "_push " + ins;
    }
    else if (ins.starts_with(":")) {
        // label
        return
            ins.substr(1) + ":";
    }
    else if (ins.starts_with("dup")) {
        return
            "_dup " + ins.substr(3);
    }
    else if (ins.starts_with("drop")) {
        return
            "_pop";
    }
    else if (ins.starts_with("!dup")) {
        return
            "_exec " + ins.substr(4);
    }
    else if (ins.starts_with("decbsp")) {
        return
            "_decbsp " + ins.substr(6);
    }
    else if (ins.starts_with("rewind")) {
        return
            "_rewind " + ins.substr(6);
    }
    else if (ins.starts_with("'")) {
        return
            "_push " + ins.substr(1);
    }
    else if (ins == ";") return "ret";
    else if (ins == "+") return "call __add";
    else if (ins == "-") return "call __sub";
    else if (ins == "*") return "call __mul";
    else if (ins == "/") return "call __div";
    else if (ins == "negate") return "call __negate";
    else {
        return
            "call " + ins;
    }
}

string fasm(const vector<string> &s) {
    string res;

    for (string ins : s) res += fasm_ins(ins) + "\n";

    return res;
}

// make it executable
string fasm(string pseudasm) {
    string res;
    stringstream ss { pseudasm };
    string buf;
    stack<unsigned> program_line_stack;
    unsigned program_line = 0;
    
    while (getline(ss, buf, '\n')) {
        vector<string> instrs = split_instr(buf);

        if (instrs.size() == 0) { res += "\n"; continue; }

        // single mms replace
        for (string &ins : instrs) {
            if (ins == "#push_here") {
                program_line_stack.push(program_line);
                goto continue_line;
            } else if (ins == "#pop") {
                ins = to_string(program_line_stack.top());
                program_line_stack.pop();
            }
            else if (builtin_functions.contains(ins)) { ins = "call __" + ins; }
            else if (ins == "$a0") { ins = "eax"; }
        }

        // multiple mms replace
        if (instrs[0] == "push") {
            instrs = { "mov", "eax", instrs[1] };
            res += join_instr(instrs) + "\n";
            
            instrs = { "call", "__push" };
            res += join_instr(instrs) + "\n";
            continue;
        }
        else if (instrs[0] == "getf") {
            instrs = { "mov", "edx", instrs[1] };
            res += join_instr(instrs) + "\n";
            
            instrs = { "call", "__getf" };
            res += join_instr(instrs) + "\n";
            continue;
        }
        else if (instrs[0] == "pushf") {
            instrs = { "mov", "eax", instrs[1] };
            res += join_instr(instrs) + "\n";
            
            instrs = { "call", "__pushf" };
            res += join_instr(instrs) + "\n";
            continue;
        }
        else if (instrs[0] == "popf") {
            instrs = { "call", "__popf" };
            res += join_instr(instrs) + "\n";
            continue;
        }
        
        res += join_instr(instrs) + "\n";

continue_line:
        program_line++;
    }

    return res;
}
