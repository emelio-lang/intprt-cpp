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
    "add", "sub", "mul", "div", "negate", "concat"
};

const map<string, unsigned> bf2arity = {
    { "negate", 1 },
    { "add", 2 },
    { "sub", 2 },
    { "mul", 2 },
    { "div", 2 },
    { "concat", 2 },
};

inline void outprg(string &res, const string &body) { res += body + "\n"; }
inline void outprg(string &res, string &env, const pair<string,string> &bodyenv) { res += bodyenv.first + "\n"; env += bodyenv.second + "\n"; }

string StaticProgram = "";
unsigned function_call_counter = 0;
unsigned conditional_counter = 0;

enum GuardType { GTYPE_COUNTABLE_FINITE, GTYPE_FINITE };
struct Guard {
    vector<pair<string, shared_ptr<Code>>> finites;
    shared_ptr<Code> countable;
};


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
            if (argstack.empty()) break;
//            cout << *argstack.top() << endl;
            bind[argname] = argstack.top()->arity;
            argstack.pop();
        }

        this->operator()(c->l->body);
        c->arity += c->l->body->arity;

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



pair<string,string>
codegen::operator () (const shared_ptr<Code> c) {

    string res = "";
    string env = "";

    for (int i = c->args.size()-1; i >= 0; i--) {
        argstack.push_back(c->args[i]);
    }

    if (c->l) {
        // instant function call

        for (auto argname : c->l->argnames) {
            if (argstack.empty()) {
                // unbinded.push_back(argname);
                return make_pair("", "");
            } else {
                cout << argname << ":" << *argstack.back() << endl << endl;
                bind[argname] = argstack.back();
                argstack.pop_back();
            }
            // defined.insert(argname);
        }

        outprg(res, env, this->operator()(c->l->body));
        
        return make_pair(res, env);
    }
    else if (c->lit.val == "fuse") {
    }
    else if (builtin_functions.contains(c->lit.val)) {
        // builtin-function call

        if (bf2arity.at(c->lit.val) != argstack.size()) return make_pair("", "");

        cout << "buitin-function call" << endl;
        cout << *c <<endl;
        cout << "current stack: ";
        SCOUT(argstack);
        
        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            outprg(res, env, this->operator()(arg));
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();
        
        outprg(res, c->lit.val);
    }
    else if (is_literal(c->lit.val)) {
        // literal

        cout << "literal" << endl;
        cout << *c <<endl;
        
        // for (auto arg : argstack) {
        //     auto _argstack = argstack;
        //     argstack = {};
        //     outprg(res, env, codegen({arg, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
        //     copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        //     argstack = _argstack;
        // }
        // argstack.clear();

        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            outprg(res, env, this->operator()(arg));
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();


        outprg(res, "push "+c->lit.val);
    }
    else {
        // general function call

        // NOTE: spill counter cuz it's grobval
        const unsigned fnidx = function_call_counter++;
        const unsigned argnum = argstack.size();
        const bool is_recursive = !bind.contains(c->lit.val);

        cout << "general function call: " << c->lit.val << endl;
        cout << "current stack: ";
        SCOUT(argstack);
        
        if (bind[c->lit.val]->arity - argnum > 0) {
            cout << "few !!" << endl;
            // copy(argstack.begin(), argstack.end(), back_inserter(bind[c->lit.val]->args));
            // outprg(res, env, this->operator()(c->l->body));
            return make_pair("","");
        }

        
        outprg(res, ";; " + c->lit.val);

        //body call
        outprg(res, env, this->operator()(bind[c->lit.val]));
        
        argstack.clear();
        return make_pair(res, env);
    }


    return make_pair(res, env);
}




deque<shared_ptr<Code>> argstack;
pair<string,string> codegen(CodegenFlow cf) {
    shared_ptr<Code> &c = cf.c;
    deque<string> &unbinded = cf.unbinded;
    set<string> &defined = cf.defined;
    deque<shared_ptr<Code>> &argstack_ = cf.argstack;
    unsigned &fstack_offset = cf.fstack_offset;
    set<string> &in_recursion = cf.in_recursion;
    map<string, shared_ptr<Code>> &bind = cf.bind;

    string res = "";
    string env = "";

    for (int i = c->args.size()-1; i >= 0; i--) {
        argstack.push_back(c->args[i]);
    }

    if (c->l) {
        // instant function call

        for (auto argname : c->l->argnames) {
            if (argstack.empty()) {
                // TODO
                unbinded.push_back(argname);
                return make_pair("", "");
            } else {
                cout << argname << ":" << *argstack.back() << endl << endl;
                bind[argname] = argstack.back();
                
                // outprg(env, "_" + argname + ": ");
                // outprg(env, env, codegen({argstack.back(), unbinded, {}, defined, fstack_offset, in_recursion, bind}));
                // outprg(env, "ret");
                argstack.pop_back();
            }
            defined.insert(argname);
        }

        outprg(res, env, codegen({c->l->body, unbinded, argstack_, defined, fstack_offset, in_recursion, bind}));

        
        // cout << *c << endl;
        // cout << make_pair(res, env) << endl << endl;

        
        return make_pair(res, env);
    }


    if (c->lit.val == "fuse") {
        // fuse

        // NOTE: only see direct arguments
        Guard guard = get_guard(c->args);
        GuardType gtype = get_guard_type(c->args);


        switch (gtype) {
            case GTYPE_COUNTABLE_FINITE: {
                // fuseのargumentは関係ないので消しておく
                // TODO: これだと、最初のstackに詰める処理が無駄...
                for (int i = 0; i < c->args.size(); i++)
                    argstack.pop_back();

                for (auto a : argstack) {
                    outprg(res, ":)");
                    stringstream ss;
                    ss << *a;
                    outprg(res, ss.str());
                    
                }

                // TODO: ここでもしargstackが空であれば、スタックから読み込むべき

                // run first arg to patternmatch
                // TODO: 関数の時は？
                outprg(res, env, codegen({argstack.back(), unbinded, {}, defined /* TODO ? */, fstack_offset, in_recursion, bind}));

                // pattern match
                outprg(res, "mov edx,1");
//                outprg(res, "call __get");
                outprg(res, "get");

                unsigned endif_index = conditional_counter + guard.finites.size();

                cout << "Guard: " << endl;
                
                for (pair<string, shared_ptr<Code>> fnt : guard.finites) {
                    outprg(res, "cmp $a0," + fnt.first);// HACKed
                    outprg(res, "jne else" + to_string(conditional_counter));
                    // outprg(res, "true"+to_string(conditional_counter)+":");

//                    outprg(res, "call __pop");
                    outprg(res, "pop");
                    outprg(res, env, codegen({fnt.second, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
                    cout << "finite " << *fnt.second << endl;
                    
                    // outprg(res, "jmp endif"+ to_string(conditional_counter));
                    // outprg(res, "false"+to_string(conditional_counter)+":");
                    outprg(res, "jmp endif"+to_string(endif_index));
                    outprg(res, "else"+to_string(conditional_counter)+":");

                    conditional_counter++;
                }

//                outprg(res, "call __pop");
                outprg(res, "pop");
                // NOTE: これより内部の引数はcodegen内で実行されるので皮だけでok
                for (auto argname : guard.countable->l->argnames)
                    defined.insert(argname);
                outprg(res, env, codegen({guard.countable, unbinded, argstack_, defined, fstack_offset, in_recursion, bind}));
                cout << "countable " << *guard.countable << endl;
                
                outprg(res, "endif"+to_string(endif_index)+":");
            } break;
                
            case GTYPE_FINITE: {
            } break;
        }

        argstack.clear();
        
    }
    else if (builtin_functions.contains(c->lit.val)) {
        // builtin-function call

        if (bf2arity.at(c->lit.val) != argstack.size()) return make_pair("", "");

        cout << "buitin-function call" << endl;
        cout << *c <<endl;
        
        for (auto arg : argstack) {
            auto _argstack = argstack;
            argstack = {};
            outprg(res, env, codegen({arg, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
            copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
            argstack = _argstack;
        }
        argstack.clear();
        
        outprg(res, c->lit.val);
    }
    else if (is_literal(c->lit.val)) {
        // literal

        cout << "literal" << endl;
        cout << *c <<endl;
        
        for (auto arg : argstack) {
            auto _argstack = argstack;
            argstack = {};
            outprg(res, env, codegen({arg, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
            copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
            argstack = _argstack;
        }
        argstack.clear();

        outprg(res, "push "+c->lit.val);
    }
    else {
        // general function call

        // NOTE: spill counter cuz it's grobval
        const unsigned fnidx = function_call_counter++;
        const unsigned argnum = argstack.size();
        const bool is_recursive = !defined.contains(c->lit.val);

        // shallow copy 
        // Code argumented = *bind[c->lit.val];
        // copy(argstack.begin(), argstack.end(), back_inserter(argumented.args));
        // argumented.arity -= argstack.size();
        // argstack.clear();
        
        if (bind[c->lit.val]->arity - argnum > 0) {
            copy(argstack.begin(), argstack.end(), back_inserter(bind[c->lit.val]->args));
            
            cout << "few !!" << endl;
            return make_pair("","");
        }

        cout << "general function call" << endl;
        cout << *c <<endl;

        
        outprg(res, ";; " + c->lit.val);

        //body call
        outprg(res, env, codegen({// make_shared<Code>(argumented)
                    bind[c->lit.val]
                                  , unbinded, argstack, defined, fstack_offset, in_recursion, bind}));
        
        argstack.clear();
    }

    // cout << *c << endl;
    // cout << make_pair(res, env) << endl << endl;

    return make_pair(res, env);
}


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
