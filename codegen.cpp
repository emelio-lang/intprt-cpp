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

#include <sstream>

const set<string> builtin_functions = {
    "add", "sub", "mul", "div", "negate", "concat"
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



pair<string,string> codegen(CodegenFlow cf) {
    shared_ptr<Code> &c = cf.c;
    deque<string> &unbinded = cf.unbinded;
    deque<shared_ptr<Code>> &argstack = cf.argstack;
    set<string> &defined = cf.defined;
    unsigned &fstack_offset = cf.fstack_offset;
    set<string> &in_recursion = cf.in_recursion;
    map<string, shared_ptr<Code>> &bind = cf.bind;

    
}


pair<string,string> codegen(CodegenFlow cf) {
    shared_ptr<Code> &c = cf.c;
    deque<string> &unbinded = cf.unbinded;
    deque<shared_ptr<Code>> &argstack = cf.argstack;
    set<string> &defined = cf.defined;
    unsigned &fstack_offset = cf.fstack_offset;
    set<string> &in_recursion = cf.in_recursion;
    map<string, shared_ptr<Code>> &bind = cf.bind;

    string res = "";
    string env = "";

    for (int i = 0; i < c->args.size(); i++) {
        argstack.push_back(c->args[i]);
    }

    if (c->l) {
        // instant function call

        for (auto argname : c->l->argnames) {
            if (argstack.empty()) {
                // TODO
                unbinded.push_back(argname);
            } else {
                bind[argname] = argstack.back();
                
                outprg(env, "_" + argname + ": ");
                outprg(env, env, codegen({argstack.back(), unbinded, {}, defined, fstack_offset, in_recursion, bind}));
                outprg(env, "ret");
                argstack.pop_back();
            }
            defined.insert(argname);
        }

        outprg(res, env, codegen({c->l->body, unbinded, argstack, defined, fstack_offset, in_recursion, bind}));

        
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
                outprg(res, env, codegen({guard.countable, unbinded, argstack, defined, fstack_offset, in_recursion, bind}));
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
        
        for (auto arg : argstack) { // TODO: direction check
            outprg(res, env, codegen({arg, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
        }
        argstack.clear();
        
        outprg(res, c->lit.val);
    }
    else if (is_literal(c->lit.val)) {
        // literal
        
        for (auto arg : argstack) {
            outprg(res, env, codegen({arg, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
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

        if (is_recursive) {
            fstack_offset += argnum;
            outprg(res, ":)");

            // TODO: no support for inner arguments
            for (auto a : bind[c->lit.val]->l->argnames)
                in_recursion.insert(a);
        }
        
        outprg(res, ";; " + c->lit.val);
        
        // NOTE: avoid redundant jump
        if (argnum != 0)
            outprg(res, "jmp F" + to_string(fnidx));

        unsigned args = 0;
        for (auto arg : argstack) {
            outprg(res, "F" + to_string(fnidx) + "." + to_string(args) + ":");
            if (is_recursive) {
                outprg(res, "mov $fsbs,1");
            }
            
            outprg(res, env, codegen({arg, unbinded, {}, defined, fstack_offset, in_recursion, bind}));
            
            if (is_recursive) outprg(res, "mov $fsbs,0");
            outprg(res, "ret");
            args++;
        }

        // push argfns into fstack
        if (argnum != 0)
            outprg(res, "F" + to_string(fnidx) + ":");
        for (unsigned i = 0; i < argnum; i++) {
            outprg(res, "pushf " "F" + to_string(fnidx) + "." + to_string(i));
        }

        // NOTE: fsbの解放はここではしない（もちろん、この先のcall先で実際には実行されるので）
        if (is_recursive) {
            outprg(res, "mov $fsb,"+to_string(fstack_offset));
            fstack_offset -= argnum;
        }

        // call actual function
        if (find(unbinded.begin(), unbinded.end(), c->lit.val) != unbinded.end()) {
            // if it's unbinded arg, let it get from fstack
            int dist = abs(distance(unbinded.end(), find(unbinded.begin(), unbinded.end(), c->lit.val)));
            // enable 'it' when the variable is in recursion
            if (in_recursion.contains(c->lit.val))
                outprg(res, "mov $fsbs,1");
            
            outprg(res, "getf " + to_string(dist));
            outprg(res, "call $a0");
            
            if (in_recursion.contains(c->lit.val))
                outprg(res, "mov $fsbs,0");
        } else {
            outprg(res, "call _" + c->lit.val);
        }

        if (is_recursive) {
            outprg(res, "mov $fsb," + to_string(fstack_offset));
        }

        // pop previously pushed argfns
        for (unsigned i = 0; i < argnum; i++) {
            outprg(res, "popf");
        }
        
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
