
string
codegen3::compress(const pair<string,string> &&v) {
    return v.second + v.first;
}

void
codegen3::paircat(pair<string,string> &x, const pair<string,string> &&v) {
    x.first += v.first;
    x.second += v.second;
}

pair<string,string>
codegen3::human_function_call(const shared_ptr<Code> c) {
    // general function call
    pair<string,string> res;

    const unsigned fnidx = function_call_counter++;

    int i = 1;
    for (auto arg : c->args) {
        res.second += "(F"+to_string(fnidx)+"A"+to_string(i)+")\n";
        res.second += compress(this->operator()(arg));
        res.first += "Push: (F"+to_string(fnidx)+"A"+to_string(i)+")\n";
        i++;
    }

    // もし、関数がそのままならここに本体を書いて良い
    if (c->l) {
        for (auto a : c->l->argnames) {
            res.first += "Pop:\n";
            res.first += "Register pointer to hash: "+a+"\n";
        }
        paircat(res, this->operator()(c->l->body));
        for (auto a : c->l->argnames) {
            res.first += "ReleasePop:\n";
        }
    } else {
        res.first += "Push spaces for return value.\n";
        res.first += "Refer: "+c->lit.val+"\n";
        res.first += "Jump to that pointer\n";
    }

    return res;
}

// R1実行. これをやると、結果的にstack topに計算結果の実値が残る.
pair<string,string>
codegen3::fnrun1(const shared_ptr<Code> c) {
    pair<string,string> res;
    const unsigned fnidx = function_call_counter++;

    res.first += "PUSHV(0);\n";
    for (int i = c->args.size()-1; i >= 0; i--) {
        // 引数もR1実行
        paircat(res, this->fnrun1(c->args[i]));
    }

    if (is_literal(c->lit.val)) {
        // literal
        assert(c->args.size() == 0);

        if (human) {
            res.first += "Push: "+c->lit.val+"\n";
        } else {
            res.first += "PUSHV("+c->lit.val+");\n";
        }
    } else if (builtin_functions.contains(c->lit.val)) {
        const map<string,string> c_builtin = {
            { "add", "0" },
            { "sub", "1" },
            { "mul", "2" },
            { "negate", "3" },
            { "div", "4" },
        };
        res.first += "HP["+c_builtin.at(c->lit.val)+"]();\n";
    }
    else {
        res.first += "HP['"+c->lit.val+"']();\n";
    }

    return res;
}

pair<string,string>
codegen3::fnrun2(const shared_ptr<Code> c) {
    pair<string,string> res;
    const unsigned fnidx = function_call_counter++;

    if (builtin_functions.contains(c->lit.val)) {
        paircat(res, this->fnrun1(c->l->body));
        return res;
    }

    for (int i = c->args.size()-1; i >= 0; i--) {
        // 引数はまとめながらラムダ抽象
        // 今はまとめはしない
        res.second += "void F"+to_string(fnidx)+"A"+to_string(i)+"() {\n";
        res.second += compress(this->fnrun2(c->args[i]));
        res.second += "return;\n";
        res.second += "}\n";
        res.first += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
    }

    if (c->l) {
        for (auto a : c->l->argnames) {
            res.first += "HP['"+a+"'] = TOP()->fp; POP();\n";
        }
        paircat(res, this->fnrun2(c->l->body));
    }
    else {
        res.first += "HP['"+c->lit.val+"']();\n";
    }
    
    return res;
}


pair<string,string>
codegen3::function_call(const shared_ptr<Code> c) {
    // general function call
    pair<string,string> res;

    const unsigned fnidx = function_call_counter++;

    res.first += "PUSHV(0);\n";
    for (int i = c->args.size()-1; i >= 0; i--) {
        res.second += "void F"+to_string(fnidx)+"A"+to_string(i)+"() {\n";
        res.second += compress(this->operator()(c->args[i]));
        res.second += "return;\n";
        res.second += "}\n";
        res.first += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
    }

    // もし、関数がそのままならここに本体を書いて良い
    if (c->l) {
        int i = 0;
        for (auto a : c->l->argnames) {
            bind[a] = this->stack_height-1;
            res.first += "HP['"+a+"'] = TOP()->fp; POP();\n";
            i++;
        }
        paircat(res, this->operator()(c->l->body));
        // for (auto a : c->l->argnames) {
        //     res.first += "MPOP();\n";
        // }

    }
    else if (builtin_functions.contains(c->lit.val)) {
        // builtin-function call

        const map<string,string> c_builtin = {
            { "add", "0" },
            { "sub", "1" },
            { "mul", "2" },
            { "negate", "3" },
            { "div", "4" },
        };

        res.first += "HP["+c_builtin.at(c->lit.val)+"]();\n";
    }
    else {
        //Push spaces for return value
        res.first += "HP['"+c->lit.val+"']();\n";
    }

    for (int i = c->args.size()-1; i >= 0; i--) {
        res.first += "MPOP();\n";
    }

    return res;
}

vector<int> hypes;

string codegen3::evoke_rel(int rel) {
    if (rel < 0) {
        return "(SP+("+to_string(rel-stack_height)+"))->fp();\n";
    } else {
        return "(MEM+("+to_string(rel-1)+"))->fp();\n";
    }
}

string codegen3::print_rel(int rel) {
    if (rel < 0) {
        return "SP+("+to_string(rel-stack_height);
    } else {
        return "MEM+("+to_string(rel-1);
    }
}

pair<string,string>
codegen3::operator () (const shared_ptr<Code> c) {
    pair<string,string> res;

    cout << *c << endl << endl;

    // else
    /*if (c->lit.val == "fuse") {
        // fuse
        // NOTE: only see direct arguments
        Guard guard = get_guard(c->args);
        GuardType gtype = get_guard_type(c->args);

        switch (gtype) {
        case GTYPE_COUNTABLE_FINITE: {
            paircat(res, this->operator ()(guard.countable));
        } break;

        case GTYPE_FINITE: {
        } break;
        }

//        argstack.clear();
    }
    else */if (is_literal(c->lit.val)) {
//        const unsigned fnidx = function_call_counter++;

//        res.second += "void LIT"+to_string(fnidx)+"() {\n";
//        res.second += "PUSHV("+c->lit.val+");\n";
//        res.second += "return;\n";
//        res.second += "}\n";

        assert(c->args.size() == 0);
        this->stack_height++;
        if (human) {
            res.first += "Push: "+c->lit.val+"\n";
        } else {
            res.first += "PUSHV("+c->lit.val+");\n";
//            res.first += "PUSHF(LIT"+to_string(fnidx)+");\n";
        }
    }
    else if (builtin_functions.contains(c->lit.val)) {
        const unsigned fnidx = function_call_counter++;

        // res.first += "PUSHV(0); POP();\n";
        if (c->lit.val == "fuse") {
            // fuse
            // NOTE: only see direct arguments
            Guard guard = get_guard(c->args);
            GuardType gtype = get_guard_type(c->args);

            switch (gtype) {
            case GTYPE_COUNTABLE_FINITE: {
                res.first += "if (false) ;\n";
                for (auto p : guard.finites) {
                    auto tmp_bind = this->bind;
                    this->stack_height = 0;
                    this->argstack = {};
                    res.first += "else if (("+print_rel(-1)+")->val=="+p.first+") {\n";
                    this->operator ()(guard.countable);
                    res.first += "}\n";
                    this->bind = tmp_bind;
                }
                res.first += "else {\n";
                auto tmp_bind = this->bind;
                paircat(res, this->operator ()(guard.countable));
                this->bind = tmp_bind;
                res.first += "}\n";
            } break;

            case GTYPE_FINITE: {
            } break;
            }

    //        argstack.clear();
        } else {

            while (!argstack.empty()) {
                res.first += evoke_rel(argstack.back());
                argstack.pop_back();
            }

            auto tmp_bind = this->bind;
            for (int i = c->args.size()-1; i >= 0; i--) {
                // 引数もR1実行
                paircat(res, this->operator()(c->args[i]));
                this->bind = tmp_bind;
            }

            const map<string,string> c_builtin = {
                { "add", "0" },
                { "sub", "1" },
                { "mul", "2" },
                { "negate", "3" },
                { "div", "4" },
            };
    //        res.first += "HP["+c_builtin.at(c->lit.val)+"]();\n";
            res.first += c->lit.val+"();\n";
            int ar = bf2arity.at(c->lit.val);
            this->stack_height -= ar-1;
        }

//        res.first += "*STACK("+to_string(ar+2)+") = *TOP();\n";
//        for (int i = ar-1 + (ar != 0 ? 1 : 0)/*いらない方の戻り値分*/; i >= 0; i--) {
//            res.first += "MPOP();\n";
//            this->stack_height--;
//        }
    }
    else {
        const unsigned fnidx = function_call_counter++;

        hypes.emplace_back(0);

        if (c->args.size() != 0 && c->arity == 0) {
            res.first += "PUSHV(0); POP();\n";
            hypes.back()++;
            this->stack_height++;
        }
        auto tmp_bind = this->bind;
        auto tmp_stackh = stack_height;
        auto tmp_stack = argstack;
        auto tmp_hypes = hypes;
//        cout << c->args.size() << " arguments {" << endl;
        for (int i = c->args.size()-1; i >= 0; i--) {
            // 引数はまとめながらラムダ抽象
            // 今はまとめはしない TODO: キャプチャーをやらないなら良さそう
            res.first += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
            tmp_stackh++;
            hypes.clear();
            tmp_hypes.back()++;
            this->stack_height = 0;
            this->argstack = {};
            tmp_stack.push_back(tmp_stackh);
            res.second += "void F"+to_string(fnidx)+"A"+to_string(i)+"() {\n";
            res.second += "union memory_t *MEM = SP;\n";
            res.second += compress(this->operator()(c->args[i]));
            res.second += "return;\n";
            res.second += "}\n";
        }
//        cout << "}" << endl;
        this->bind = tmp_bind;
        this->stack_height = tmp_stackh;
        this->argstack = tmp_stack;
        hypes = tmp_hypes;

        // ここでhypeが解決されるかどうか
        // 解決されるなら0にしておいて以降のbodyを生成する
        // 奥から、解決条件で累積していくみたいな処理
        bool beHypeResolved = c->args.size() != 0 && c->arity == 0;
        int local_hype = hypes.size();

        if (c->l) {
            int i = 0;
            for (auto a : c->l->argnames) {
                if (this->argstack.empty()) {
                    i++;
                    bind[a] = -i;
                } else {
                    bind[a] = this->argstack.back();
                    this->argstack.pop_back();
                }
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
            //     res.first += c->lit.val+"();\n";
            // } else
            if (!bind.contains(c->lit.val)) {
                res.first += c->lit.val + "(";

                ");\n";
            }
            res.first += evoke_rel(bind[c->lit.val]);
            this->stack_height++;

            cout << "ll" << endl;
            cout << res.first << endl;
        }

        // 戻り地コピー
        // TODO: pointerより大きい時は、ヒープに実体を置いてとかしないと駄目かもね...
        if (beHypeResolved) {
            if (c->l) {
                // 明日：hypeをメンバにして、解決するまでどんどん足していく。ここで解決。
                res.first += "*STACK("+to_string(hypes.back()+1)+") = *TOP();\n";
                // 部分適用なら（c->args.size() == c->l->body->arityでないなら）、
                // MPOPはc->l->body->arityを発行する
                for (int i = hypes.back()/*TODO*/; i >= 1; i--) {
                    res.first += "MPOP();\n";
                    this->stack_height--;
                }
            } else {
                res.first += "*STACK("+to_string(1+2)+") = *TOP();\n";
                // NOTE: ここは内容不明のhikisuu文字。
                // (|a b| a 3)とか
                // arityは種システムを制作してarityを保証してから
                // 今は値と信じて１と決め打ちしておく

                for (int i = 1 + (1 == 0 ? 0 : 1)/*いらない方の戻り値分*/; i >= 1; i--) {
                    res.first += "MPOP();\n";
                    this->stack_height--;
                }
            }
            hypes.pop_back();
        }
    }

    // cout << res.first << endl << res.second << endl;
    return res;
}
