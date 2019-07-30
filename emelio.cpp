#include <iostream>
#include <string>
#include <stack>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"

#include "emelio.h"

typedef string Type;

struct Fn {
    pair<vector<Type>, vector<Type>> typ;
    Code body;
    vector<Token> argnames = {};
};

struct Code {
    Fn& fn;
    vector<Code> args;

    // 試験的に計算結果をここに入れるようにしてみる
    // リテラルもここに保存。リテラルの時はfn = __literal
    string cal = "";
};

Tokenizer tkn;
Fn   __literal = Fn { {{},{}}, {Token{"__literal"}} }
    ,__main = Fn { {{},{}}, {Token{"__main"}} };
unsigned long noname_counter = 0;
map<string, Fn> bind = {
    { "add",    Fn { {{"Int","Int"},{"Int"}}, {Token{"--add"}} } },
    { "negate", Fn { {{"Int"},{"Int"}}, {Token{"--negate"}} } },
    { "let",    Fn { {{"_Bind", "#Symbol"},{"()", "_Bind"}}, {Token{"--let"}} } },
};
Code maincode = Code { __main, {} };

void showCode(Code& code, int lvl = 0) {
    cout << string("  ") * lvl << "Code: ";
    for (Token t : code.fn.body) cout << t.val;
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


Code parse_code(vector<Token>& tkns) {
    stack<Code*> codest = {};
    Code res = /*{fn args cal}*/ Code {bind[tkn.tokenvals[0]], {}};
    codest.push(&res);
    int mode = 0; /*
                    0 -- initial
                    1 -- new function
                    2 -- new argument
                  */
    mode = 2;
    
    for (int i = 2; i < tkn.tokenvals.size(); ++i) {
        switch (mode) {
            case 1:
                if (tkn.tokenvals[i] == ")") continue;

                // here, it is a function or a name of function

                Fn& parsed = parse_fn(i);
                codest.top()->args.push_back(Code {parsed, {}});
                codest.push(&(*--codest.top()->args.end()));
                mode = 2;
                break;
             
            case 2:
                if (tkn.tokenvals[i] == "(") {
                    mode = 1;
                } else if (tkn.tokenvals[i] == ")") {
                    codest.pop();
                } else {
                    codest.top()->args.push_back(lit_code(tkn.tokens[i]));
                }
                break;
        }
    }

    return res;
}


Fn& refer_fn(string name) {
    // refer to bind table and store function's reference
    if (!CONTAINS(bind, name)) {
        cout << "[Error] undefinied function \"" << tkn.tokenval[i] << "\""
             << " (" << tkn.token[i].line << ":" << tkn.token[i].col << ")" << endl;
        return;
    }

    return bind[name];
}


Fn& parse_fn(int &idx) {
    Fn res = Fn { {{}, {}}, {} };
    // 名前だけのときもある
    if (tkn.tokenvals[idx] == '(') {
        idx++;
        int mode = 0; /*
                        1 -- args
                        
                       */
        
        if (tkn.tokenvals[idx] == '|') {
            idx++;
            for (; idx < tkn.tokenvals.size(); idx++) {
                if (tkn.tokenvals[idx] == '|')
                    goto success1;

                res.argnames.push_back(tkn.token[idx]);
            }

            cout << "[Error] No corresponding |" << endl;
success1:
        }

        // TODO: subvectorをもう一度構築しているので
        // メモリ的により効率的な方法がありますか？
        vector<Token> fn_body_tkns = {};
        
        for (; idx < tkn.tokenvals.size(); idx++) {
            if (tkn.token[idx] == ')') {
                res.body = parse_code(fn_body_tkns);
                goto success2;
            }

            fn_body_tkns.push_back(tkn.token[idx]);
        }
        
        cout << "[Error] No corresponding (" << endl;
        
success2:
        bind.push_back(make_pair("__noname" + noname_counter, res));

        return bind["__noname" + noname_counter];
    } else {
        // TODO: 名前参照
        return refer_fn(tkn.tokenvals[idx]);
    }
}

int exec_code(Code& code) {
    
}


void exec() {
    stack<Code*> codest = {};
    int mode = 0; /*
                    0 -- initial
                    1 -- new function
                    2 -- new argument
                  */
    maincode.args.push_back(Code {bind[tkn.tokenvals[1]], {}});
    codest.push(&(*--maincode.args.end()));
    mode = 2;
    
    for (int i = 2; i < tkn.tokenvals.size(); ++i) {
        switch (mode) {
            case 1:
                if (tkn.tokenvals[i] == ")") continue;

                // here, it is a function or a name of function

                Fn& parsed = parse_fn(i);
                codest.top()->args.push_back(Code {parsed, {}});
                codest.push(&(*--codest.top()->args.end()));
                mode = 2;
            break;
             
            case 2:
                if (tkn.tokenvals[i] == "(") {
                    mode = 1;
                } else if (tkn.tokenvals[i] == ")") {
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
                    else {
                        for (string argname : t->fn.argnames) {
                            
                        }
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

    showv(tkn.tokenvals);

    exec();

    showProgram();

    cout << maincode.args[0].cal << endl;

    return 0;
}
