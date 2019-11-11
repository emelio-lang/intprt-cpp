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

#include <set>
#include <sstream>

const set<string> builtin_functions = {
    "add", "negate", "concat"
};

inline void outprg(string &res, const string &body) { res += body + "\n"; }
inline void outprg(string &res, string &env, const pair<string,string> &bodyenv) { res += bodyenv.first + "\n"; env += bodyenv.second + "\n"; }

string StaticProgram = "";
unsigned function_call_counter = 0;

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

pair<string,string> codegen(shared_ptr<Code> c,
                            deque<string> unbinded,
                            vector<shared_ptr<Code>> argstack
                            ) {
    string res = "";
    string env = "";

    if (c->l) {
        // anonymous function call
        for (int i = c->args.size()-1; i >= 0; --i) {
            argstack.push_back(c->args[i]);
        }

        for (auto argname : c->l->argnames) {
            if (argstack.empty()) {
                // TODO
                unbinded.push_back(argname);
//                outprg(res, codegen(argstack.top(), unbinded));
            } else {
                outprg(env, "_" + argname + ": ");
                outprg(env, env, codegen(argstack.back(), unbinded));
                outprg(env, "ret");
                argstack.pop_back();
            }
        }

        outprg(res, env, codegen(c->l->body, unbinded, argstack));
        
        return make_pair(res, env);
    }

    if (builtin_functions.contains(c->lit.val)) {
        // builtin-function call
        
        for (auto arg : c->args) {
            outprg(res, env, codegen(arg, unbinded));
        }
        for (auto arg : argstack) { // TODO: direction check
            outprg(res, env, codegen(arg, unbinded));
        }
        argstack.clear();
        
        outprg(res, c->lit.val);
        // outprg(res, "pop $1");
        // outprg(res, "pop $2");
        // outprg(res, "add $1, $1, $2");
        // outprg(res, "push $1");
    } else if (is_literal(c->lit.val)) {
        // literal
        
        for (auto arg : c->args) {
            outprg(res, env, codegen(arg, unbinded));
        }
        for (auto arg : argstack) {
            outprg(res, env, codegen(arg, unbinded));
        }
        argstack.clear();

        outprg(res, "push "+c->lit.val);
    } else {
        // general function call

        // NOTE: spill counter cuz it's grobval
        unsigned fnidx = function_call_counter++;
        
        outprg(res, "jmp F" + to_string(fnidx));

        unsigned args = 0;
        for (auto arg : c->args) {
            outprg(res, "F" + to_string(fnidx) + "." + to_string(args) + ":");
            outprg(res, env, codegen(arg, unbinded));
            outprg(res, "ret");
            args++;
        }
        for (auto arg : argstack) {
            outprg(res, "F" + to_string(fnidx) + "." + to_string(args) + ":");
            outprg(res, env, codegen(arg, unbinded));
            outprg(res, "ret");
            args++;
        }

        // push argfns into fstack
        outprg(res, "F" + to_string(fnidx) + ":");
        for (unsigned i = 0, len = c->args.size() + argstack.size(); i < len; i++) {
            outprg(res, "pushf " "F" + to_string(fnidx) + "." + to_string(i));
        }

        // call actual function
        if (find(unbinded.begin(), unbinded.end(), c->lit.val) != unbinded.end()) {
            // if it's unbinded arg, let it get from fstack
            int dist = abs(distance(unbinded.end(), find(unbinded.begin(), unbinded.end(), c->lit.val)));
            outprg(res, "getf " + to_string(dist));
            outprg(res, "call $a0");
        } else {
            outprg(res, "call _" + c->lit.val);
        }

        // pop previously pushed argfns
        for (unsigned i = 0, len = c->args.size() + argstack.size(); i < len; i++) {
            outprg(res, "popf");
        }
        
        argstack.clear();
    }

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
