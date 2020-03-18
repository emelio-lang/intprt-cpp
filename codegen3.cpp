#include "emelio.h"
#include "util.h"
#include "notation.h"
#include "codegen.h"

#include <sstream>
string
codegen3::compress(const Compiled &&v) {
    return v.body + v.env;
}

void
codegen3::paircat(Compiled &x, const Compiled &&v) {
    x.body += v.body;
    x.env += v.env;
}

void operator+=(Compiled& lhs, const Compiled&& rhs) {
    lhs.body += rhs.body;
    lhs.env += rhs.env;
}

void operator<<=(string& lhs, const Compiled&& rhs) {
    lhs += rhs.body + rhs.env;
}

vector<int> hypes;

// Offsetがrelのスタック内容を実行する
Compiled codegen3::evoke_rel(int rel) {
    if (rel < 0) {
        return Compiled {"(SP+("+to_string(rel-stack_height)+"))->fp();\n"};
    } else {
        return Compiled {"(MEM+("+to_string(rel-1)+"))->fp();\n"};
    }
}

// Offsetがrelのスタック内容をコピーする
Compiled codegen3::copy_rel(int rel) {
    return Compiled {"PUSHV(("+print_rel(rel)+")->val);\n"};
}

// Offsetがrelのスタック内容
string codegen3::print_rel(int rel) {
    if (rel < 0) {
        return "SP+("+to_string(rel-stack_height)+")";
    } else {
        return "MEM+("+to_string(rel-1)+")";
    }
}

Compiled
codegen3::literal(const string lit) {
    if (is_number(lit)) {
        return Compiled {"PUSHV("+lit+");\n"};
    } else {
        return Compiled {"PUSHS("+lit+");\n"};
    }
}


Compiled
codegen3::fuse(const vector<shared_ptr<Code>> fns) {
    Compiled res;

    // NOTE: only see direct arguments
    Guard guard = get_guard(fns);
    GuardType gtype = get_guard_type(fns);

    switch (gtype) {
    case GTYPE_COUNTABLE_FINITE: {
        res.body += "if (false) ;\n";
        for (auto p : guard.finites) {
            auto tmp_bind = this->bind;
            this->stack_height = 0;
            this->argstack = {};
            res.body += "else if (("+print_rel(-1)+")->val=="+p.first+") {\n";
            res += this->operator ()(p.second);
            res.body += "}\n";
            this->bind = tmp_bind;
        }
        res.body += "else {\n";
        auto tmp_bind = this->bind;
        res += this->operator ()(guard.countable);
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
codegen3::builtin(const string &name) {
    Compiled res;

    const map<string,string> c_builtin = {
        { "add", "0" },
        { "sub", "1" },
        { "mul", "2" },
        { "negate", "3" },
        { "div", "4" },
    };
    res.body += name+"();\n";
    int ar = bf2arity.at(name);
    this->stack_height -= ar-1;

    return res;
}

Compiled
codegen3::all_arguments_evoked(const vector<shared_ptr<Code>> &args) {
    Compiled res;

    while (!argstack.empty()) {
        if (argstack_code.back()->arity == 0) {
            res += copy_rel(argstack.back());
        } else {
            res += evoke_rel(argstack.back());
        }
        argstack.pop_back();
        argstack_code.pop_back();
    }

    auto tmp_bind = this->bind;
    for (int i = args.size()-1; i >= 0; i--) {
        // 引数もR1実行
        res += this->operator()(args[i]);
        this->bind = tmp_bind;
    }

    return res;
}

Compiled
codegen3::argument_evoked(const shared_ptr<Code> &c) {
    Compiled res;

    auto tmp_bind = this->bind;
    res += this->operator()(c);
    this->bind = tmp_bind;

    return res;
}



//Compiled
//codegen3::argument_holded() {
//    while (!argstack.empty()) {
//        res.body += evoke_rel(argstack.back());
//        argstack.pop_back();
//    }

//    auto tmp_bind = this->bind;
//    for (int i = c->args.size()-1; i >= 0; i--) {
//        // 引数もR1実行
//        paircat(res, this->operator()(c->args[i]));
//        this->bind = tmp_bind;
//    }
//}


Compiled
codegen3::operator () (const shared_ptr<Code> c) {
    Compiled res;

    cout << *c << endl << endl;

    if (is_literal(c->lit.val)) {
//        const unsigned fnidx = function_call_counter++;

//        res.env += "void LIT"+to_string(fnidx)+"() {\n";
//        res.env += "PUSHV("+c->lit.val+");\n";
//        res.env += "return;\n";
//        res.env += "}\n";

        assert(c->args.size() == 0);
        this->stack_height++;
        res = literal(c->lit.val);
    }
    else if (builtin_functions.contains(c->lit.val)) {
        // res.body += "PUSHV(0); POP();\n";
        if (c->lit.val == "fuse") {
            res = fuse(c->args);
        } else {
            res += all_arguments_evoked(c->args);

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
        const unsigned fnidx = function_call_counter++;

        hypes.emplace_back(0);

        if (c->args.size() != 0 && c->arity == 0) {
            res.body += "PUSHV(0); POP();\n";
            hypes.back()++;
            this->stack_height++;
        }

        map<int, int> binded_with;  // keyの位置にbindされるべきargnameのindex
        if (c->l) {
            auto tmp_stack = argstack;
            auto tmp_stackh = stack_height;
            for (int i = c->args.size()-1; i >= 0; i--) {
                tmp_stackh++;
                tmp_stack.emplace_back(tmp_stackh);
            }

            int i = 0, j =0;
            for (auto a : c->l->argnames) {
                if (tmp_stack.empty()) {
                    i++;
//                    bind[a] = BindInfo{-i};
                } else {
                    binded_with[tmp_stack.back()] = j;
//                    bind[a] = BindInfo{tmp_stack.back()};
                    tmp_stack.pop_back();
                }
                j++;

            }
        }

        auto tmp_bind = this->bind;
        auto tmp_stackh = stack_height;
        auto tmp_stack = argstack;
        auto tmp_stack_code = argstack_code;
        auto tmp_hypes = hypes;
//        cout << c->args.size() << " arguments {" << endl;
        for (int i = c->args.size()-1; i >= 0; i--) {
            // 引数はまとめながらラムダ抽象 - (HACK: arityが0ならR1実行)
            // 今はまとめはしない TODO: キャプチャーをやらないなら良さそう
            tmp_stackh++;
            hypes.clear();
            tmp_hypes.back()++;
            this->stack_height = 0;
            this->argstack = {};
            tmp_stack.emplace_back(tmp_stackh);
            tmp_stack_code.emplace_back(c->args[i]);

            if (c->args[i]->arity == 0) {
                res += this->operator()(c->args[i]);
            } else {
                int binded_with_arg = binded_with[tmp_stackh];
                res.body += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
                res.env += "void F"+to_string(fnidx)+"A"+to_string(i)+"() {\n";
                res.env += "union memory_t *MEM = SP;\n";
                if (c->l && c->l->argqualities[binded_with_arg].recursive) {
                    // in_recursive.insert(c->l->argnames[binded_with_arg]);
                    res.env += "static void(*"+c->l->argnames[binded_with_arg]+")() = F"+to_string(fnidx)+"A"+to_string(i)+";\n";
                }
                res.env <<= this->operator()(c->args[i]);
                res.env += "return;\n";
                res.env += "}\n";
            }
        }
//        cout << "}" << endl;
        this->bind = tmp_bind;
        this->stack_height = tmp_stackh;
        this->argstack = tmp_stack;
        this->argstack_code = tmp_stack_code;
        hypes = tmp_hypes;

        // ここでhypeが解決されるかどうか
        // 解決されるなら0にしておいて以降のbodyを生成する
        // 奥から、解決条件で累積していくみたいな処理
        bool beHypeResolved = c->args.size() != 0 && c->arity == 0;
        int local_hype = hypes.size();

        if (c->l) {
            int i = 0, j=0;
            for (auto a : c->l->argnames) {
                if (this->argstack.empty()) {
                    i++;
                    bind[a] = -i;
                    bindinfo[a] = BindInfo{nullptr};
                } else {
                    bind[a] = this->argstack.back();
                    bindinfo[a] = BindInfo{this->argstack_code.back()};
                    this->argstack.pop_back();
                    this->argstack_code.pop_back();
                }
                j++;
            }

            auto tmp_bind = this->bind;
            paircat(res, this->operator()(c->l->body));
            if (beHypeResolved) {
                for (int i = hypes.size()-1; i >= local_hype; i--) {
                    hypes[local_hype-1] += hypes.back();
                    hypes.pop_back();
                }
            }
            this->bind = tmp_bind;
        }
        else {
            // 文字参照

            // if (builtin_functions.contains(c->lit.val)) {
            //     res.body += c->lit.val+"();\n";
            // } else
            if (!bind.contains(c->lit.val)) {
                // とりあえず何もミスってなければ再帰
                res.body += c->lit.val + "(";

                res.body += ");\n";
            } else if (!bindinfo[c->lit.val].code || bindinfo[c->lit.val].code->arity == 0) {
                res += copy_rel(bind[c->lit.val]);
            } else {
                res += evoke_rel(bind[c->lit.val]);
            }
            this->stack_height++;

            cout << "ll" << endl;
            cout << res.body << endl;
        }

        // 戻り地コピー
        // TODO: pointerより大きい時は、ヒープに実体を置いてとかしないと駄目かもね...
        if (beHypeResolved) {
            if (c->l) {
                // 明日：hypeをメンバにして、解決するまでどんどん足していく。ここで解決。
                res.body += "*STACK("+to_string(hypes.back()+1)+") = *TOP();\n";
                // 部分適用なら（c->args.size() == c->l->body->arityでないなら）、
                // MPOPはc->l->body->arityを発行する
                for (int i = hypes.back()/*TODO*/; i >= 1; i--) {
                    res.body += "MPOP();\n";
                    this->stack_height--;
                }
            } else {
                res.body += "*STACK("+to_string(1+2)+") = *TOP();\n";
                // NOTE: ここは内容不明のhikisuu文字。
                // (|a b| a 3)とか
                // arityは種システムを制作してarityを保証してから
                // 今は値と信じて１と決め打ちしておく

                for (int i = 1 + (1 == 0 ? 0 : 1)/*いらない方の戻り値分*/; i >= 1; i--) {
                    res.body += "MPOP();\n";
                    this->stack_height--;
                }
            }
            hypes.pop_back();
        }
    }

    // cout << res.body << endl << res.env << endl;
    return res;
}
