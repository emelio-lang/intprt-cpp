/***
 * codegen ver.3
 * C備え付けのスタックと、一本のスタックを用いて計算する
 *
 * (|args ...|
 *     body
 * ) a b c ...
 * 関数と引数らで構成されるコードを翻訳する。
 * 基本は、まずは関数のarityを確かめて関数が食う分の引数だけ実行時メモリに実体を作らせてポインタを積む、
 * 内部でargstackに残りの引数は入れておく。関数内部へと移る前に内部で束縛の処理をやっておく。
 * できたら、bodyを同様に翻訳していく。argstackは食う引数を云々してる時に足りない場合に補充する(部分適用)。
 *
 * (|x| add x) 3 7
 *
 * 例えば上のコードを翻訳していく。[実行時スタック]{束縛情報}[argstack]で状況を書いていく。
 * まずは、外側の引数の３、７をargstackに積む。
 * []{}[7 3]
 * xを取る関数がarity1であることを確認してargstackを削って、束縛の処理及び実行時メモリに実体を作る。
 * [3]{x=3}[7]
 * ここで内部に移る。ここでは関数がaddで引数がx。まずは引数をargstackへ
 * [3]{x=3}[x 7]
 * addがarity2であることを確認して、２つ食う。
 * [3 7 x]{x=3}[]
 * addが実行され、
 * [3 10]{x=3}[]
 * 関数抜け出し（束縛していたのはxだったので、これを開放して戻り値をコピー）
 * [10]{}[]
 ***/


#include "emelio.h"
#include "util.h"
#include "notation.h"
#include "codegen.h"

#include <sstream>
string
codegen5::compress(const Compiled &&v) {
    return v.body + v.env;
}

void
codegen5::paircat(Compiled &x, const Compiled &&v) {
    x.body += v.body;
    x.env += v.env;
}

// Offsetがrelのスタック内容を実行する
Compiled codegen5::evoke_rel(int rel) {
    if (rel < 0) {
        return Compiled {"(SP+("+to_string(rel-stack_height)+"))->fp();\n"};
    } else {
        return Compiled {"(MEM+("+to_string(rel-1)+"))->fp();\n"};
    }
}

// Offsetがrelのスタック内容をコピーする
Compiled codegen5::copy_rel(int rel) {
    return Compiled {"PUSHV(("+print_rel(rel)+")->val);\n"};
}

// Offsetがrelのスタック内容
// relがマイナスのときは直前の関数呼び出しまでの引数を表す
// 関数呼び出し時にはstack_heightは0に初期化され、そこからまた追跡を始める
string codegen5::print_rel(int rel) {
    if (rel < 0) {
        return "SP+("+to_string(rel-stack_height)+")";
    } else {
        return "MEM+("+to_string(rel-1)+")";
    }
}

Compiled
codegen5::literal(const string lit) {
    if (is_number(lit)) {
        return Compiled {string(lit)};
    } else {
        return Compiled {"\""+lit+"\""};
    }
}

Compiled
codegen5::fuse(const vector<shared_ptr<Code>> fns) {
    Compiled res;
    // NOTE: only see direct arguments
    Guard guard = get_guard(fns);
    GuardType gtype = get_guard_type(fns);

    switch (gtype) {
    case GTYPE_COUNTABLE_FINITE: {
        // TODO:
        // (|x| ... )
        // (|3| ... )
        // がfuseされていた時、引数は比較用に実体化する（計算する）が、計算した後3の方に行ったらxは使えないので最初の時点で開放するのもありかも（節約）

        const unsigned fnidx = function_call_counter++;
        int i = 0, j=0;

        for (auto a : guard.countable->l->argnames) {
            if (this->argstack.empty()) {
                i++;
                bind[a] = -i;
                bindinfo[a] = BindInfo{nullptr};

                reserved_bind[a] = reserved.back();
                reserved.pop_back();
            } else {
                auto arg = this->argstack.back();

                res.body += "int " + a + " = ";
                res += argument_compiled(a, arg.second);
                res.body += ";\n";
                this->argstack.pop_back();

                bind[a] = stack_height;
                bindinfo[a] = BindInfo{arg.second};
            }
            j++;
        }

        // fuseされた関数の呼び出しは (...) 3 のような即席関数のように感じますが、実行時にしかわからない比較がついてくるので関数呼び出しのような形に

        res.body += "if (false) ;\n";
        for (auto p : guard.finites) {
            // TODO: fuse の即席実行では-1ではない。。。
            res.body += "else if ("+guard.countable->l->argnames[0]+" == "+p.first+") {\n";
            {
                auto tmp_bind = bind;
                auto tmp_stack = argstack;
                auto tmp_stackh = stack_height;
                argstack = {};
                res += codegen5::operator()(p.second, guard.countable->l, true);
                bind = tmp_bind;
                argstack = tmp_stack;
                stack_height = tmp_stackh;
            }
            res.body += "}\n";
        }

        res.body += "else {\n";
        auto tmp_bind = this->bind;
        res += this->operator ()(guard.countable->l->body, guard.countable->l, true);
        this->bind = tmp_bind;
        res.body += "}\n";
    } break;

    case GTYPE_FINITE: {
    } break;
    }

    //        argstack.clear();
    return res;
}

Compiled
codegen5::builtin(const string &name) {
    Compiled res;
    const map<string,string> c_builtin = {
        { "add", "+" },
        { "sub", "-" },
        { "mul", "*" },
        { "negate", "-" },
        { "div", "/" },
    };

    if (argstack.size() < bf2arity.at(name)) return res;

    if (bf2arity.at(name) == 2) {
        auto arg1 = argstack.front(); argstack.pop_front();
        auto arg2 = argstack.front(); argstack.pop_front();
        res += argument_evoked(arg1.second);
        res.body += c_builtin.at(name);
        res += argument_evoked(arg2.second);
    } else if (bf2arity.at(name) == 1) {
        auto arg1 = argstack.front(); argstack.pop_front();
        res.body += c_builtin.at(name);
        res += argument_evoked(arg1.second);
    }

    int ar = bf2arity.at(name);
    this->stack_height -= ar-1;

    return res;
}

// arguments list
Compiled
codegen5::all_arguments_evoked() {
    Compiled res;

    auto tmp_bind = bind;
    auto tmp_argstack = argstack;
    argstack = {};
    while (!tmp_argstack.empty()) {
        auto arg = tmp_argstack.front();
        // ほんとうはarg.secondコンパイルする前に自身はpopbackする
        // argとして参照があるので消えないはず
        tmp_argstack.pop_front();
        res += this->operator()(arg.second, nullptr, false);
        res.body += ",";
    }
    bind = tmp_bind;
    argstack = {};

    // NOTE: 最後のコンマを消す
    res.body.erase(res.body.end()-1);

    return res;
}

Compiled
codegen5::argument_evoked(const shared_ptr<Code> &arg) {
    Compiled res;

    auto tmp_bind = bind;
    auto tmp_argstack = argstack;
    argstack = {};

    res += this->operator()(arg, nullptr, false);

    bind = tmp_bind;
    argstack = tmp_argstack;

    return res;
}

string
codegen5::argdef(const TypeSignature &typesig, bool reserve) {
    const unsigned fnidx = function_call_counter++;
    string res = "";

    for (int i = 0; i < typesig.from.size(); i++) {
        res += vardef("__" + to_string(fnidx) + "a" + to_string(i), typesig.from[i]) + ",";
        if (reserve) reserved.emplace_back("__" + to_string(fnidx) + "a" + to_string(i));
    }
    res.erase(res.end()-1);

    return res;
}

string
codegen5::vardef(const string &name, const TypeSignature &typesig) {
    const unsigned fnidx = function_call_counter++;
    string res = "";
    const auto normalized = typesig.normalized();

    if (typesig.from.size() == 0) {
        res += get<string>(normalized.to) + " " + name;
    } else {
        res += get<string>(normalized.to) + "(* " + name + ") (";
        argdef(typesig, false);
    }
    return res;
}

//reserveを考慮した名前
string
codegen5::get_name(string name) {
    if (reserved_bind.contains(name)) {
        return reserved_bind.at(name);
    }
    return name;
}

// NOTE: argstackからとったなら解放忘れずに
// argとして引数となるCodeを渡すと、関数呼び出しの際に引数の実体化として一般的に行われる処理をargで行います
Compiled
codegen5::argument_compiled(const string &ident , const shared_ptr<Code> &arg) {
    Compiled res;

    if (arg->arity == 0) {
        {
            auto tmp_bind = bind;
            auto tmp_stack = argstack;
            argstack = {};
            res.body += "int " + ident + " = ";
            res.body <<= codegen5::operator()(arg, nullptr, false);
            res.body += ";\n";
            bind = tmp_bind;
            argstack = tmp_stack;
        }
//                        res.body += "PUSHV("+arg.second->lit.val+");\n";
    } else {
//        res.body += ident + "();\n";
//        stack_height++;

        const auto normalized = arg->type.normalized();
        res.env += get<string>(normalized.to) + " " + ident + "(" + argdef(normalized, true) + ") {";

//                if (c->l && c->l->argqualities[binded_with_arg].recursive) {
//                    // in_recursive.insert(c->l->argnames[binded_with_arg]);
//                    res.env += "static void(*"+c->l->argnames[binded_with_arg]+")() = F"+to_string(fnidx)+"A"+to_string(i)+";\n";
//                }
        {
            auto tmp_bind = bind;
            auto tmp_stackh = stack_height;
            auto tmp_stack = argstack;
            argstack = {};
            stack_height = 0;
            res.env <<= codegen5::operator()(arg, nullptr, true);
            bind = tmp_bind;
            stack_height = tmp_stackh;
            argstack = tmp_stack;
        }
        res.env += "}\n";
    }

    return res;
}

// TODO: いづれ型情報とかつけるときのことも考えてこれはスマートじゃないので、Lambdaとかを渡したほうが良い気がする
// とにかく、codeを処理するのにその上の引数確保情報も必要ということ
// そんなのcallして帰ってきてからで良いやんという意見もあるけど、名前callの場合面倒だし、そちらで綺麗に解決しないのがhypeを作らないといけなくなった根源かも
Compiled
codegen5::operator () (const shared_ptr<Code> c, const shared_ptr<Lambda> envl, bool whole_function) {
    Compiled res;

    cout << *c << endl << endl;

    const unsigned fnidx = function_call_counter++;
    for (int i = 0; i < c->args.size(); i++) {
        if (c->args[i]->arity == 0) {
            argstack.emplace_back(make_pair("", c->args[i]));
        } else {
            //int binded_with_arg = binded_with[tmp_stackh];
            argstack.emplace_back(make_pair("F"+to_string(fnidx)+"A"+to_string(i), c->args[i]));
//                res.body += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";

        }
    }


    if (is_literal(c->lit.val)) {
//        const unsigned fnidx = function_call_counter++;

//        res.env += "void LIT"+to_string(fnidx)+"() {\n";
//        res.env += "PUSHV("+c->lit.val+");\n";
//        res.env += "return;\n";
//        res.env += "}\n";

        assert(c->args.size() == 0);
        this->stack_height++;
        if (whole_function) res.body += "return ";
        res += literal(c->lit.val);
        if (whole_function) res.body += ";\n";
    }
    else if (builtin_functions.contains(c->lit.val)) {
        // res.body += "PUSHV(0); POP();\n";
        if (c->lit.val == "fuse") {
            // 無駄にargstack積んでるので崩す
            for (int i = 0; i < c->args.size(); i++) {
                argstack.pop_back();
            }
            res = fuse(c->args);
        } else {
            res += builtin(c->lit.val);
    //        res.body += "HP["+c_builtin.at(c->lit.val)+"]();\n";
        }

//        res.body += "*STACK("+to_string(ar+2)+") = *TOP();\n";
//        for (int i = ar-1 + (ar != 0 ? 1 : 0)/*いらない方の戻り値分*/; i >= 0; i--) {
//            res.body += "MPOP();\n";
//            this->stack_height--;
//        }
    }
    else {
        if (c->l) {
            // 即時関数

            int i = 0, j=0;
            for (auto a : c->l->argnames) {
                if (this->argstack.empty()) {
                    i++;
                    bind[a] = -i;
                    bindinfo[a] = BindInfo{nullptr};

//                    reserved_bind[a] = reserved.back();
//                    reserved.pop_back();
                } else {
                    auto arg = this->argstack.back();

                    // 実体を作って、whole_function呼び
                    // 関数なら、envを外部に作る
                    res += argument_compiled(a, arg.second);

                    this->argstack.pop_back();

                    bind[a] = stack_height;
                    bindinfo[a] = BindInfo{arg.second};
                }
                j++;
            }

            auto tmp_bind = this->bind;
            if (whole_function) res.body += "return ";
            res += this->operator()(c->l->body, c->l, false);
            if (whole_function) res.body += ";\n";
            this->bind = tmp_bind;
        }
        else {
            // 文字参照

            // if (builtin_functions.contains(c->lit.val)) {
            //     res.body += c->lit.val+"();\n";
            // } else
            if (!bind.contains(c->lit.val)) {

                // TODO: 引数のコンパイル

                // とりあえず何もミスってなければ再帰
                res.body += get_name(c->lit.val) + "(";
                for (int i = argstack.size()-1; i >= 0; i--) {
                    auto arg = this->argstack.front();

                    res += argument_evoked(arg.second);
                    this->argstack.pop_front();
                    res.body += ",";
                }
                // NOTE: 最後のコンマを消す
                res.body.erase(res.body.end()-1);
                res.body += ")";
            // 参照先コードのarityを見て、0なら値をコピってスタックにのっけるだけ
            } else if (!bindinfo[c->lit.val].code || bindinfo[c->lit.val].code->arity == 0) {
//                res += copy_rel(bind[c->lit.val]);
                res.body += get_name(c->lit.val);
                stack_height++;
            // それ以外なら、ちゃんとcallする
            } else {
                res.body += get_name(c->lit.val) + "(";
                for (int i = bindinfo[c->lit.val].code->arity-1; i >= 0; i--) {
                    auto arg = this->argstack.front();

                    res += argument_evoked(arg.second);
                    this->argstack.pop_front();
                    res.body += ",";
                }
                // NOTE: 最後のコンマを消す
                res.body.erase(res.body.end()-1);
                res.body += ")";

//                res += evoke_rel(bind[c->lit.val]);
                stack_height++;
            }

            cout << "ll" << endl;
            cout << res.body << endl;
        }
    }

    // 戻り値コピーv.2 envlを用いたい!
    if (envl) {
        if (envl->argnames.size() != 0)
        // *STACK(1) = *TOP()は実際何の効果もないので省く
        {
//            res.body += "*STACK("+to_string(envl->argnames.size()+1)+") = *TOP();\n";
        }
        for (int i = 0; i < (int)envl->argnames.size(); i++) {
//            res.body += "MPOP();\n";
            this->stack_height--;
        }
    }

    // cout << res.body << endl << res.env << endl;
    return res;
}
