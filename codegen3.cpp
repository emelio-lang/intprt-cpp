
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

pair<string,string>
codegen3::operator () (const shared_ptr<Code> c) {
    pair<string,string> res;

    // else
    if (c->lit.val == "fuse") {
        // fuse
        // NOTE: only see direct arguments
        Guard guard = get_guard(c->args);
        GuardType gtype = get_guard_type(c->args);

        switch (gtype) {
            case GTYPE_COUNTABLE_FINITE: {
            } break;
                
            case GTYPE_FINITE: {
            } break;
        }

//        argstack.clear();
    }
    else if (is_literal(c->lit.val)) {
        assert(c->args.size() == 0);
        this->stack_height++;
        if (human) {
            res.first += "Push: "+c->lit.val+"\n";
        } else {
            res.first += "PUSHV("+c->lit.val+");\n";
        }
    }
    else if (builtin_functions.contains(c->lit.val)) {
        const unsigned fnidx = function_call_counter++;

//        res.first += "PUSHV(0); POP();\n";
        auto tmp_bind = this->bind;
        for (int i = 0; i < c->args.size(); i++) {
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
        res.first += "HP["+c_builtin.at(c->lit.val)+"]();\n";
        this->stack_height -= bf2arity.at(c->lit.val)-1;
    }
    else {
        const unsigned fnidx = function_call_counter++;

        if (c->args.size() != 0) {
            res.first += "PUSHV(0); POP();\n";
            this->stack_height++;
        }
        auto tmp_bind = this->bind;
        auto tmp_stackh = stack_height;
        auto tmp_stack = argstack;
        for (int i = c->args.size()-1; i >= 0; i--) {
            // 引数はまとめながらラムダ抽象
            // 今はまとめはしない
            cout << "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
            cout << tmp_stackh<<"\n";
            
            res.first += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
            tmp_stackh++;
            this->stack_height = 0;
            this->argstack = {};
            tmp_stack.push_back(tmp_stackh);
            res.second += "void F"+to_string(fnidx)+"A"+to_string(i)+"() {\n";
            res.second += compress(this->operator()(c->args[i]));
            res.second += "return;\n";
            res.second += "}\n";
        }
        this->bind = tmp_bind;
        this->stack_height = tmp_stackh;
        this->argstack = tmp_stack;

        if (c->l) {
            int i = 0;
            for (auto a : c->l->argnames) {
                cout << a << endl;
                for (auto e : argstack) {
                    cout << e << " ";
                }
                cout << endl;
                if (this->argstack.empty()) {
                    i++;
                    bind[a] = -i;
                } else {
                    cout << this->argstack.back() << endl;
                    bind[a] = this->argstack.back();
                    this->argstack.pop_back();
                }
//                res.first += "HP['"+a+"'] = TOP()->fp; POP();\n";
            }
            auto tmp_bind = this->bind;
            paircat(res, this->operator()(c->l->body));
            this->bind = tmp_bind;
        }
        else {
            if (bind[c->lit.val] < 0) {
                res.first += "(SP+("+to_string(bind[c->lit.val]-stack_height)+"))->fp();\n";
            } else {
                res.first += "(MEM+("+to_string(bind[c->lit.val]-1)+"))->fp();\n";
            }
            this->stack_height++;
        }

        // 戻り地コピー
        // TODO: pointerより大きい時は、ヒープに実体を置いてとかしないと駄目かもね...
        if (c->args.size() != 0)
            res.first += "*STACK("+to_string(c->args.size()+2)+") = *TOP();\n";
        for (int i = c->args.size()-1 + (c->args.size() != 0 ? 1 : 0)/*いらない方の戻り値分*/; i >= 0; i--) {
            res.first += "MPOP();\n";
            this->stack_height--;
        }
    }

    return res;
}


// pair<string,string>
// codegen3::operator () (const shared_ptr<Code> c) {
//     pair<string,string> res;

//     // else
//     if (c->lit.val == "fuse") {
//         // fuse

//         // NOTE: only see direct arguments
//         Guard guard = get_guard(c->args);
//         GuardType gtype = get_guard_type(c->args);

//         switch (gtype) {
//             case GTYPE_COUNTABLE_FINITE: {
//             } break;

//             case GTYPE_FINITE: {
//             } break;
//         }

//         argstack.clear();
//     }
//     else if (is_literal(c->lit.val)) {
//         assert(c->args.size() == 0);
//         this->stack_height++;
//         if (human) {
//             res.first += "Push: "+c->lit.val+"\n";
//         } else {
//             res.first += "PUSHV("+c->lit.val+");\n";
//         }
//     }
//     else if (builtin_functions.contains(c->lit.val)) {
//         const unsigned fnidx = function_call_counter++;

// //        res.first += "PUSHV(0); POP();\n";
//         auto tmp_bind = this->bind;
//         for (int i = 0; i < c->args.size(); i++) {
//             // 引数もR1実行
//             paircat(res, this->operator()(c->args[i]));
//             this->bind = tmp_bind;
//         }

//         const map<string,string> c_builtin = {
//             { "add", "0" },
//             { "sub", "1" },
//             { "mul", "2" },
//             { "negate", "3" },
//             { "div", "4" },
//         };
//         res.first += "HP["+c_builtin.at(c->lit.val)+"]();\n";
//         this->stack_height -= bf2arity.at(c->lit.val)-1;
//     }
//     else {
//         const unsigned fnidx = function_call_counter++;

//         if (c->args.size() != 0) {
//             res.first += "PUSHV(0); POP();\n";
//             this->stack_height++;
//         }
//         auto tmp_bind = this->bind;
//         auto tmp_stack = stack_height;
//         for (int i = c->args.size()-1; i >= 0; i--) {
//             // 引数はまとめながらラムダ抽象
//             // 今はまとめはしない
//             res.first += "PUSHF(F"+to_string(fnidx)+"A"+to_string(i)+");\n";
//             tmp_stack++;
//             res.second += "void F"+to_string(fnidx)+"A"+to_string(i)+"() {\n";
//             this->stack_height = tmp_stack;
//             argstack.push_back(this->stack_height);
//             res.second += compress(this->operator()(c->args[i]));
//             res.second += "return;\n";
//             res.second += "}\n";
//         }
//         this->bind = tmp_bind;
//         this->stack_height = tmp_stack;

//         if (c->l) {
//             int i = 0;
//             for (auto a : c->l->argnames) {
//                 bind[a] = this->stack_height-i;
//                 res.first += "HP['"+a+"'] = TOP()->fp; POP();\n";
//                 i++;
//             }
//             auto tmp_bind = this->bind;
//             paircat(res, this->operator()(c->l->body));
//             this->bind = tmp_bind;
//         }
//         else {
// //            res.first += "HP['"+c->lit.val+"']();\n";
//             cout << c->lit.val << endl;
//             cout << bind[c->lit.val] << endl;
//             cout << this->stack_height << endl;
//             res.first += "(MEM+"+to_string(bind[c->lit.val]-1)+")->fp();\n";
//             this->stack_height++;
//         }

//         // 戻り地コピー
//         // TODO: pointerより大きい時は、ヒープに実体を置いてとかしないと駄目かもね...
//         if (c->args.size() != 0)
//             res.first += "*STACK("+to_string(c->args.size()+2)+") = *TOP();\n";
//         for (int i = c->args.size()-1 + (c->args.size() != 0 ? 1 : 0)/*いらない方の戻り値分*/; i >= 0; i--) {
//             res.first += "MPOP();\n";
//             this->argstack.pop_back();
//             this->stack_height--;
//         }
//     }

//     return res;
// }
