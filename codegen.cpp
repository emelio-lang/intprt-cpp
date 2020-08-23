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

map<string, TypeSignature> set_type::data_bind;

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
    { "negate", make_shared<TypeFn>( TypeFn { {"int"}, "int" } )},
    { "add", make_shared<TypeFn>( TypeFn { {"int","int"}, "int" } )},
    { "sub", make_shared<TypeFn>( TypeFn { {"int","int"}, "int" } )},
    { "mul", make_shared<TypeFn>( TypeFn { {"int","int"}, "int" } )},
    { "div", make_shared<TypeFn>( TypeFn { {"int","int"}, "int" } )},
    { "concat", make_shared<TypeFn>( TypeFn { {"string","string"}, "string" } )},
    { "_get", make_shared<TypeFn>( TypeFn { {"any"}, "any" } )},
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

Guard get_guard(const deque<shared_ptr<Code>> &args) {
    Guard res;
    for (auto arg : args) {
        if (is_literal(arg->l->argnames[0])) {
            res.finites.push_back(make_pair(arg->l->argnames[0], arg->l->body));
        } else {
            res.countables.emplace_back(arg);
        }
    }
    return res;
}

GuardType get_guard_type(const deque<shared_ptr<Code>> &args) {
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

void set_fv::operator () (const shared_ptr<Code> c) {
    // if (c->l) {
    //     set_fv()(c->l->body);
    //     c->l->freevars = c->l->body->l ? c->l->body->l->freevars
    //     for (auto arg : c->args) {
    //         c->l->
    //     }
    // } else {
    // }
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
    cout << *c << endl;

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
            TypeSignature a = normalized(bind[argname]);
            TypeSignature b = normalized(c->args[i]->type);
            ASSERT(verify(a, b),
                   ("引数"+argname+"の型が合致しません. (expected:" + to_string(a) +
                    " inferred:"+to_string(b)+")"));
        }

        // 4
        deep_copy_from(c->l->type, c->l->body->type);
        deep_copy_from(c->type, c->l->body->type);
        // cout << to_string(c->type) << endl;
        // cout << "wrap " << c->l->argtypes.size() << endl;
        for (int i = 0; i < c->l->argtypes.size(); ++i) {
            wrap(c->l->type, c->l->argtypes[i]);
            wrap(c->type, c->l->argtypes[i]);
        }
        // cout << "apply " << c->args.size() << endl;
        apply(c->type, (int) c->args.size());
    }
    else if (c->lit.val == "_get") {
        (set_type(&bind))(c->args[0]);
        auto type_name = PURE(string)(c->args[0]->type);
        if (MATCHS(TypeProduct)(data_bind[type_name])) {
            auto &records = PURES(TypeProduct)(data_bind[type_name]);
            string varname = "";
            {
                auto &pivot = c->args[1];
                while (pivot->l) { pivot = pivot->l->body; }
                varname = pivot->lit.val;
            }
            auto &fruit = records->products[distance(records->names.begin(), find(records->names.begin(), records->names.end(), varname))];
            deep_copy_from(c->type, fruit);

            // shared_ptr<TypeFn> newfn(new TypeFn);
            // deep_copy_from(newfn->to, fruit);
            // {
            //     TypeSignature tmp;
            //     deep_copy_from(tmp, type_name);
            //     newfn->from.emplace_back(tmp);
            // }
//            c->type = fruit;
        } else {
            assert("TODO: あとで！");
        }
    }
    else if (c->lit.val == "fuse") {
        c->type = shared_ptr<TypeSum>(new TypeSum);

        for (int i = c->args.size()-1; i >= 0; i--) {
            (set_type(&bind))(c->args[i]);
            // argstack.push(c->args[i]);
            PURES(TypeSum)(c->type)->add_type(c->args[i]->type);
        }

        if (PURES(TypeSum)(c->type)->sums.size() == 1)
            c->type = c->args[0]->type;
        else
            c->type = sumfactor(PURES(TypeSum)(c->type));
    }
    else if (c->lit.val == "type") {
        shared_ptr<TypeFn> new_type_fn = shared_ptr<TypeFn>(new TypeFn);
        new_type_fn->to = c->args[0]->lit.val;
        for (auto typ : PURES(TypeProduct)(c->args[1]->rawtype)->products) {
            new_type_fn->from.emplace_back(typ);
        }
        bind[c->args[0]->lit.val] = new_type_fn;
        data_bind[c->args[0]->lit.val] = c->args[1]->rawtype;
        cout << c->args[0]->lit.val << " " << to_string(bind[c->args[0]->lit.val]) << endl;

        // tmp.normalize();
        // int i = 0;
        // for (auto typ : c->args[1]->l->argtypes) {
        //     // NOTE: c->args[1]->lへの参照を使用しています！！
        //     tmp.from.push_back(make_shared<TypeSignature>(typ));

        //     TypeSignature memfn;
        //     memfn.from.push_back(tmp.to);
        //     memfn.to = typ.to;
        //     memfn.normalize();
        //     bind[c->args[1]->l->argnames[i]] = memfn;
        //     i++;
        // }
        
//        type_constructors[c->args[0]->lit.val] = c->args[1]->l;
        this->operator()(c->args[2]);
    } else if (builtin_functions.contains(c->lit.val)) {
        for (auto a : c->args) (set_type(&bind))(a);

        deep_copy_from(c->type, bf2typesig.at(c->lit.val));
        apply(c->type, (int) c->args.size());

        int i = 0;
        for (auto a : c->args) {
            cout << to_string(arg(bf2typesig.at(c->lit.val),i)) << endl;
            TypeSignature x = normalized(arg(bf2typesig.at(c->lit.val),i));
            TypeSignature y = normalized(c->args[i]->type);
            ASSERT(verify(x, y),
                   "'"+c->lit.val+"'の"+to_string(i)+"番目の引数の型が合致しません. (expected:" + to_string(bf2typesig.at(c->lit.val)) +
                   " inferred:"+to_string(c->args[i]->type)+")");
        }
    } else if (is_literal(c->lit.val)) {
        if (is_number(c->lit.val)) {
            c->type = "int";
        }
    } else {
        const auto fnname = c->lit.val;
        for (auto a : c->args) (set_type(&bind))(a);

        // もし、関数の型が分かっていないならとりまエラー
        ASSERT(bind.contains(fnname), "'"+fnname+"' の型が不明です");

        deep_copy_from(c->type, bind[fnname]);
        apply(c->type, (int) c->args.size());

        int i = 0;
        for (auto a : c->args) {
            ASSERT(verify(normalized(arg(bind[fnname],i)), normalized(c->args[i]->type)),
                   "'"+c->lit.val+"'の"+to_string(i)+"番目の引数の型が合致しません. (expected:" + to_string(bind[fnname]) +
                   " inferred:"+to_string(c->args[i]->type)+")");
        }
    }
    cout << * c << endl;

//    cout << "a" << endl;
}

// #include "codegen1.cpp"
// #include "codegen2.cpp"
// #include "codegen3.cpp"
// #include "codegen4.cpp"
// #include "codegen5.cpp"
#include "codegen7.cpp"
//#include "ocamlgen.cpp"

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
