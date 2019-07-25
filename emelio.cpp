#include <iostream>
#include <string>
#include <stack>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"

#include "emelio.h"

typedef string Type;
typedef string Token;

struct Fn {
    pair<vector<Type>, vector<Type>> typ;
    vector<Token> body;
};

struct Code {
    Fn& fn;
    vector<Code> args;

    // 試験的に計算結果をここに入れるようにしてみる
    // リテラルもここに保存。リテラルの時はfn = __literal
    string cal = "";
};

Tokenizer tkn;
Fn   __literal = Fn { {{},{}}, {"__literal"} }
    ,__main = Fn { {{},{}}, {"__main"} };
map<string, Fn> bind = {
    { "add",    Fn { {{"Int","Int"},{"Int"}}, {"--add"} } },
    { "negate", Fn { {{"Int"},{"Int"}}, {"--negate"} } },
    { "let",    Fn { {{"_Bind", "#Symbol"},{"()", "_Bind"}}, {"--let"} } },
};
Code maincode = Code { __main, {} };

// Code eval(Code code) {
//     if (is_number(code.name)) return code;
    
//     if (!CONTAINS(bind, code.name)) {
//         cout << "[Error] undefinied function \"" << code.name << "\"" << endl;
//         return Code { "error", {} };
//     }

//     if (code.name == "add") {
//         // ここで評価
//         int a1 = eval_as_int(code.args[0]);
//         int a2 = eval_as_int(code.args[1]);

//         return Code { to_string(a1 + a2), {} };
//     } else if (code.name == "negate") {
//         int a1 = eval_as_int(code.args[0]);
        
//         return Code { to_string(-a1), {} };
//     } else if (code.name == "let") {
        
//     }
// }

void showCode(Code& code, int lvl = 0) {
    cout << string("  ") * lvl << "Code: ";
    for (Token t : code.fn.body) cout << t;
    cout << endl;
    for (Code arg : code.args) showCode(arg, lvl+1);
}

void showProgram() { showCode(maincode, 0); }

Code lit_code(string l) {
    return Code {__literal, {}, l};
}

int lit_int(Code code) {
    if (code.cal == "") {
        cout << "Something wrong happened." << endl;
        cout << "Code hasn't been calculated: " << endl;
        showCode(code);
        cout << endl;
    }

    // type check function for Int
    if (!is_number(code.cal)) {
        cout << "[ERR] Expected Int, not '" << code.cal << "'"<< endl;
        showCode(code);
        cout << endl;
    }

    return stoi(code.cal);
}


void exec() {
    stack<Code*> codest = {};
    int mode = 0; /*
                    0 -- initial
                    1 -- new function
                    2 -- new argument
                  */
    maincode.args.push_back(Code {bind[tkn.tokens[1]], {}});
    codest.push(&(*--maincode.args.end()));
    mode = 2;
    
    for (int i = 2; i < tkn.tokens.size(); ++i) {
        switch (mode) {
            case 1:
                if (tkn.tokens[i] == ")") continue;

                // here, it is a function or a name of function
                if (tkn.tokens[i] == "(") {
                    // idk functions!
                } else {
                    // refer to bind table and store function's reference
                    if (!CONTAINS(bind, tkn.tokens[i])) {
                        cout << "[Error] undefinied function \"" << tkn.tokens[i] << "\"" << endl;
                        return;
                    }

                    codest.top()->args.push_back(Code {bind[tkn.tokens[i]], {}});
                    codest.push(&(*--codest.top()->args.end()));
                    mode = 2;
                }
            break;
             
            case 2:
                if (tkn.tokens[i] == "(") {
                    mode = 1;
                } else if (tkn.tokens[i] == ")") {
                    Code* t = codest.top();

                    // TODO: 関数だけの型チェックとかはできんの？

                    if (REFEQUAL(t->fn, bind["negate"])) {
                        int a1 = lit_int(t->args[0]);
        
                        t->cal = to_string(-a1);
                    }
                    else if (REFEQUAL(t->fn, bind["add"])) {
                        int a1 = lit_int(t->args[0]);
                        int a2 = lit_int(t->args[1]);

                        t->cal = to_string(a1 + a2);
                    }
                    codest.pop();
                } else {
                    codest.top()->args.push_back(lit_code(tkn.tokens[i]));
                }
            break;
        }
    }
}


int main() {
    tkn.preset("cpp");
    tkn.tokenize_file("test.em");

    showv(tkn.tokens);

    exec();

    showProgram();

    cout << maincode.args[0].cal << endl;

    return 0;
}
