#include <iostream>
#include <fstream>
#include <string>
#include <stack>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"

#include "emelio.h"

#define NULL_FN ("__null")

typedef string Type;

/*
NOTE: 束縛情報はグローバル変数bindがずっと管理してて、それが増えたり減ったりする (TODOほんとにそれで良い？)

TODO(明日): Nullable referenceをCodeに追加して型的なミスマッチを解消する

NOTE: 少し細かい関数の表記の定義
関数は、変数参照(TODOこんなふうに言うのも変だしref関数を考えるのもあり)はそのシンボルのトークンのみ、
　　　　即時関数は'('から始まり、対応する')'までを言う。
コード（式）とは、"関数 引数..."とか"リテラル"とかプログラムとして受け取れるの形をしているものを言う(TODOは？)。
example)
(3)       関数（即時関数）
3         式
add 2 3   式
add       関数
(add 2 3) 式, 関数

 */

struct Fn;
struct Code;

struct Code {
    // NOTE: mapは再配置があるらしいので、そのバグかもしれない。
    // とりあえずキーを保存するように変えてみる
    string fnname;
    vector<Code> args;

    // 試験的に計算結果をここに入れるようにしてみる
    // リテラルもここに保存。リテラルの時はfn = __literal
    string cal = "";

    // 追加の束縛(このコード以下は

    Fn* fn();
};

struct Fn {
    pair<vector<Type>, vector<Type>> typ;
    Code body;
    vector<Token> argnames = {};
};

Tokenizer tkn;
/* Fn  // __literal = Fn { {{},{}}, Code {0, {}, } } //リテラル用 */
/* ; */
unsigned long noname_counter = 0;
map<string, Fn> bind = {
    { "add",    Fn { {{"Int","Int"},{"Int"}}, Code {NULL_FN, {}, "__+"} }},
    { "negate", Fn { {{"Int"},{"Int"}}, Code {NULL_FN, {}, "__-"}}},
    { "let",    Fn { {{"_Bind", "#Symbol"},{"()", "_Bind"}}, Code {NULL_FN, {}, "__let"}} },
};

/*********************/
/*FUNCTION PROTOTYPES*/
/*********************/
Code lit_code(string l);
int lit_int(Code code);
//Fn *refer_fn(string name);
Code parse_code(vector<Token>& tkns);
string parse_fn(vector<Token>& tkns, int &idx);
// Code exec_code(Code, int dbg = 0);
void test(bool verbose = false);

Fn* Code::fn() { return &bind[this->fnname]; }

void showCode(Code& code, int lvl=0)
{
    if (code.fnname == NULL_FN) {
        cout << code.cal;
    } else {
        cout << string("  ") * lvl;
        if (code.fn()->argnames.size() != 0) {
            cout << "\\";
            for (auto argname : code.fn()->argnames) {
                cout << argname.val << " ";
            }
        }
        showCode(code.fn()->body, lvl);
    }
//    cout << endl;

    if (code.args.size() != 0) {
        cout << "[";

        for (Code arg : code.args) {
            showCode(arg, lvl+1);
            cout << " ";
        }
        cout <<  "]" << endl;
    }
}

Code lit_code(string l)
{
    return Code {NULL_FN, {}, l};
}



Fn *refer_fn(string name)
{
    // refer to bind table and store function's reference

    return &bind[name];
}

Code parse_code(vector<Token>& tkns)
{
    int idx = 0;
    stack<Code*> codest = {};
    
    Code res;
    // NOTE: 先に書き換えの処理を行っておくこと(notation!) 1+2とかでバグる
    if (is_literal(tkns[idx].val)) {
        res = lit_code(tkns[idx].val);
        return res;
    } else {
        res = /*{fn args cal}*/ Code {parse_fn(tkns, idx), {}};
    }
    codest.push(&res);
    int mode = 0; /*
                    0 -- initial
                    1 -- new function
                    2 -- new argument
                  */
    mode = 2;
    
    for (idx++; idx < tkns.size(); ++idx) {
        switch (mode) {
            case 1: {
                // if (tkns[idx].val == ")") continue;

                // here, it is a function or a name of function

                string parsed = parse_fn(tkns, idx);
                codest.top()->args.push_back(Code {parsed, {}});
                codest.push(&(*--codest.top()->args.end()));
                mode = 2;
                idx--;
            }    break;
             
            case 2: {
                if (tkns[idx].val == "(") {
                    mode = 1;
                    idx--;
                    // TODO ここi--いる？
                } else if (tkns[idx].val == ")") {
                    codest.pop();
                } else {
                    codest.top()->args.push_back(lit_code(tkns[idx].val));
                }
            }    break;
        }
    }

    return res;
}


/* NOTE:
   変数の場合はそのまま返します(TODO これがやるべきことか？)
   関数の最初（つまり'('から）のidxを受け取って
   関数の最後のトークンまで（つまり')'まで）idxを移動させて返却します（一個次のトークンまでではないです
*/
string parse_fn(vector<Token>& tkns, int &idx)
{
    Fn res = Fn { {{}, {}}, {} };
    // 名前だけのときもある
    if (tkns[idx].val == "(") {
        idx++;
        int mode = 0; /*
                        1 -- args
                        
                      */

        // NOTE: |がネストすることはないよね？
        if (tkns[idx].val == "|") {
            idx++;
            for (; idx < tkns.size(); idx++) {
                if (tkns[idx].val == "|") {
                    idx++;
                    goto success1;
                }
                
                res.argnames.push_back(tkns[idx]);
            }

            cout << "[Error] No corresponding |" << endl;
        }
success1:


        // TODO: subvectorをもう一度構築しているので
        // メモリ的により効率的な方法がありますか？
        vector<Token> fn_body_tkns = {};
        int paren = 0;
        
        for (; idx < tkns.size(); idx++) {
            if (tkns[idx].val == "(") paren++;
            if (tkns[idx].val == ")") {
                paren--;
                if (paren == -1) {
                    res.body = parse_code(fn_body_tkns);
                    goto success2;
                }
            }

            fn_body_tkns.push_back(tkns[idx]);
        }
        
        cout << "[Error] No corresponding (" << endl;
        
success2:
        noname_counter++;
        bind.insert(make_pair("__noname" + to_string(noname_counter), res));

        return "__noname" + to_string(noname_counter);
    }
    else {
        // // TODO: 名前参照
        // Fn* refn = refer_fn(tkns[idx].val);
        // if (!refn) {
        //     cout << "[Error] undefinied function \"" << tkns[idx].val << "\""
        //          << " (" << tkns[idx].line << ":" << tkns[idx].col << ")" << endl;
        //     return 0;
        // }
        // return refn;
        return tkns[idx].val;
    }
}

Code exec_code(Code code, int dbg = 0, bool trace = false)
{
    if (trace) {
        cout << string(" ") * dbg << "Calculating... ";
        showCode(code);
        cout << endl;
    }
    
    // 変数なら、今は計算済みなはず(TODOﾎﾝﾄ？関数を含んでいる変数は？)なので解決して返す
    // リテラルなら何もしない
    if (code.fnname == NULL_FN) {
        if (!is_number(code.cal)) {
            if (bind[code.cal].body.fnname == NULL_FN) {
                code.cal = bind[code.cal].body.cal;
            } else {
                code = bind[code.cal].body; // TODO: 試験的
            }
        }

        if (trace) {
            cout << string(" ") * dbg << "done.";
            showCode(code);
            cout << endl;
        }

        return code;
    }
                                            
    if (code.fnname == "negate") {
        int a1 = lit_int(code.args[0]);
        
        code.cal = to_string(-a1);
    }
    else if (code.fnname == "add") {
        int a1 = lit_int(code.args[0]);
        int a2 = lit_int(code.args[1]);

        code.cal = to_string(a1 + a2);
    }
    else {
        /* 個別定義された関数の場合
           
           引数は計算されているはず（TODOそうでもない場合も扱えるように！）
           なので、関数のbodyで引数を全て置き換えたCodeを新たに作ってそれを実行した結果を返す。
         */

        Code applied = code.fn()->body;
        map<string, Fn> tmp_bind = bind;

        for (int i = 0;
             i < code.fn()->argnames.size();
             i++)
        {
            code.args[i] = exec_code(code.args[i], dbg+1);
        }

        for (int i = 0;
             i < code.fn()->argnames.size();
             i++)
        {
            bind[code.fn()->argnames[i].val] = Fn {
                /*TODO:試験的*/{{},{"Int"}}, /*typ*/
                lit_code(code.args[i].cal), /*code*/
                {} /*argnames*/
            };
        }


        code = exec_code(applied, dbg+1);

        bind = tmp_bind;
    }

    if (trace)
        cout << string(" ") * dbg << "done. " << code.cal << endl;
    return code;
}

int lit_int(Code code)
{
    // ここで計算されてなくても、次のexec_codeで計算されるので大丈夫？
    // if (code.cal == "") {
    //     cout << "Something wrong happened." << endl;
    //     cout << "Code hasn't been calculated: " << endl;
    //     showCode(code);
    //     cout << endl;
    // }

    
    // TODO: 試験的にここで変数を展開
    code = exec_code(code);

    // type check function for Int
    if (!is_number(code.cal)) {
        cout << "[ERR] Expected Int, not '" << code.cal << "'"<< endl;
        showCode(code);
        cout << endl;
    }

    return stoi(code.cal);
}



int main() {
    tkn.preset("cpp");
    tkn.tokenize_file("test.em");

    showv(tkn.tokenvals);

    Code maincode = parse_code(tkn.tokens);

    showCode(maincode);

    maincode = exec_code(maincode, 0, true);

    cout << endl << endl;

    cout << maincode.cal << endl;

    test();

    return 0;
}

void test(bool verbose) {
    ifstream ifs("testcases.em");

    string cd, ans, ito, kara;

    if (verbose) cout << "wwwwwww TESTING wwwwwwww" << endl;


    while (true) {
        if (!getline(ifs, ito)) break;
        if (!getline(ifs, cd)) break;
        if (!getline(ifs, ans)) break;
        getline(ifs, kara);
        
        if (verbose) {
            cout << ito << endl;
            cout << cd << endl;
        } else {
            cout << "Testing " << cd << " --- ";
        }

        tkn.tokenize(cd);
        try {
            string res = exec_code(parse_code(tkn.tokens)).cal;

            if (res == ans) {
                cout << "\033[1;32mPassed.\033[0m" << endl;
            } else {
                cout << "\033[1;31mError!\033[0m " << endl << "Expected '" << ans << "', but '" << res << "'." << endl;
                cout << ito << endl;
            }
        } 
        catch (std::exception& e)
        {
            cerr << "\033[1;31mError!\033[0m " << endl << "Exception caught : " << e.what() << endl;
            cout << ito << endl;
        }


    }
}
