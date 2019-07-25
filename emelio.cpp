#include <iostream>
#include <string>
#include <stack>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"

#include "emelio.h"

typedef string Type;
typedef string Token;

struct Code {
    string name;
    vector<Code> args;
};

struct Fn {
    pair<vector<Type>, vector<Type>> typ;
    vector<Token> body;
};

Tokenizer tkn;
map<string, Fn> bind = {
    { "add",    Fn { {{"Int","Int"},{"Int"}}, {""} } },
    { "negate", Fn { {{"Int"},{"Int"}}, {""} } },
    { "let",    Fn { {{"_Bind", "#Symbol"},{"()", "_Bind"}}, {""} } },
};
Code maincode = Code { "main", {} };

int eval_as_int(Code code);

Code eval(Code code) {
    if (is_number(code.name)) return code;
    
    if (!CONTAINS(bind, code.name)) {
        cout << "[Error] undefinied function \"" << code.name << "\"" << endl;
        return Code { "error", {} };
    }

    if (code.name == "add") {
        // ここで評価
        int a1 = eval_as_int(code.args[0]);
        int a2 = eval_as_int(code.args[1]);

        return Code { to_string(a1 + a2), {} };
    } else if (code.name == "negate") {
        int a1 = eval_as_int(code.args[0]);
        
        return Code { to_string(-a1), {} };
    } else if (code.name == "let") {
        
    }
}

int eval_as_int(Code code) {
    return stoi(eval(code).name);
}


void parse() {
    stack<Code*> codest = {};
    int mode = 0; /*
                    0 -- initial
                    1 -- new function
                    2 -- new argument
                   */
    maincode.args.push_back(Code {tkn.tokens[1], {}});
    codest.push(&(*--maincode.args.end()));
    mode = 2;
    
    for (int i = 2; i < tkn.tokens.size(); ++i) {
        switch (mode) {
            case 1:
                if (tkn.tokens[i] == ")") continue;

                codest.top()->args.push_back(Code {tkn.tokens[i], {}});
                codest.push(&(*--codest.top()->args.end()));
                mode = 2;

            break;
             
            case 2:
                if (tkn.tokens[i] == "(") {
                    mode = 1;
                } else if (tkn.tokens[i] == ")") {
                    codest.pop();
                } else {
                    codest.top()->args.push_back(Code {tkn.tokens[i], {}});
                }
            break;
        }
    }
}

void showCode(Code& code, int lvl) {
    cout << string("  ") * lvl << "Code: " << code.name << endl;
    for (Code arg : code.args) showCode(arg, lvl+1);
}

void showProgram() { showCode(maincode, 0); }


int main() {
    tkn.preset("cpp");
    tkn.tokenize_file("test.em");

    showv(tkn.tokens);

    parse();

    showProgram();

    cout << eval_as_int(maincode.args[0]) << endl;

    return 0;
}
