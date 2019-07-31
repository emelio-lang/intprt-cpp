#include <iostream>
#include <string>
#include <stack>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"

#include "emelio.h"

typedef string Type;

/*
NOTE: 束縛情報はグローバル変数bindがずっと管理してて、それが増えたり減ったりする (TODOほんとにそれで良い？)
TODO(明日): Nullable referenceをCodeに追加して型的なミスマッチを解消する
 */

struct Fn;
struct Code;

struct Code {
    Fn fn;
    vector<Code> args;

    // 試験的に計算結果をここに入れるようにしてみる
    // リテラルもここに保存。リテラルの時はfn = __literal
    string cal = "";

    // 追加の束縛(このコード以下は
};

struct Fn {
    pair<vector<Type>, vector<Type>> typ;
    Code body;
    vector<Token> argnames = {};
};

Tokenizer tkn;
Fn   __literal = Fn { {{},{}}, {Token{"__literal",0,0}} } //リテラル用
,__main = Fn { {{},{}}, {Token{"__main"}} } //
,__blank = Fn { {{},{}}, {Token{"__blank"}} } 
;
unsigned long noname_counter = 0;
map<string, Fn> bind = {
    { "add",    Fn { {{"Int","Int"},{"Int"}}, {Token{"--add"}} } },
    { "negate", Fn { {{"Int"},{"Int"}}, {Token{"--negate"}} } },
    { "let",    Fn { {{"_Bind", "#Symbol"},{"()", "_Bind"}}, {Token{"--let"}} } },
};
// TODO: 一時用のリテラル用束縛情報, Fnとうまく扱う方法を考えてbindと統合して欲しい
map<string, Code&> litbind = {
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



Fn& refer_fn(string name) {
    // refer to bind table and store function's reference
    if (!CONTAINS(bind, name)) {
        cout << "[Error] undefinied function \"" << tkn.tokenval[i] << "\""
             << " (" << tkn.token[i].line << ":" << tkn.token[i].col << ")" << endl;
        return;
    }

    return bind[name];
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
                    if (is_number(tkn.tokens[i])) {
                        codest.top()->args.push_back(lit_code(tkn.tokens[i]));
                    } else {
                        // 関数を表しているときもある
                        codest.top()->args.push_back(Code {__blank, {}, tkn.tokens[i]});
                    }
                }
                break;
        }
    }

    return res;
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

Code exec_code(Code code) {
    // 変数なら、今は計算済みなはず(TODOﾎﾝﾄ？関数を含んでいる変数は？)なので解決して返す
    if (REFEQUAL(code.fn, __blank))
        return lit_code(litbind[code.cal]);
                                            
    // 計算済みなら計算することはない
    if (code.cal != "") return code;


    if (REFEQUAL(code.fn, bind["negate"])) {
        int a1 = lit_int(code.args[0]);
        
        code.cal = to_string(-a1);
    }
    else if (REFEQUAL(code.fn, bind["add"])) {
        int a1 = lit_int(code.args[0]);
        int a2 = lit_int(code.args[1]);

        code.cal = to_string(a1 + a2);
    }
    else {
        /* 個別定義された関数の場合
           
           引数は計算されているはず（TODOそうでもない場合も扱えるように！）
           なので、関数のbodyで引数を全て置き換えたCodeを新たに作ってそれを実行した結果を返す。
         */

        Code applied = code.fn.body;
        map<string, Code&> tmp_litbind = litbind;

        for (int i = 0;
             i < code.fn.argnames;
             i++)
        {
            
            litbind[code.fn.argnames[i]] = code.args[i];
        }

        code = exec_code(applied);

        litbind = tmp_litbind;
    }

    return code;
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

    parse_code(tkn);
    exec_code(tkn);

    showProgram();

    cout << maincode.args[0].cal << endl;

    return 0;
}
