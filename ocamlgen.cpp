map<string, TypeSignature> ocamlgen::data_bind; // TODO: スコープいいの？

// TODO: 名前をつける関数のトップレベルに引数が全てない場合ダメになる
// (|foo| ...) ((|a b| ...))みたいなやつとか

string
ocamlgen::compress(const Compiled &&v) {
    return v.body + v.env;
}
void
ocamlgen::paircat(Compiled &x, const Compiled &&v) {
    x.body += v.body;
    x.env += v.env;
}

string
ocamlgen::ocaml_type_name(string s) {
    return "t_" + tolower(s);
}

string
ocamlgen::print_data_structure(const TypeSignature type) {
    string res = "";
    cout << "heyyy" << endl;
    cout << to_string(type) << endl;
    if (MATCHS(TypeProduct)(type)) {
        auto prod = PURES(TypeProduct)(type);
        for (int i = 0; i < prod->names.size(); i++) {
            res += prod->names[i] + " : " + to_string(prod->products[i]) + "; ";
        }
    }
    return res;
}


string
ocamlgen::print_type_from(const deque<TypeSignature> &tys, const shared_ptr<Lambda> &lam) {
    string res="";
    for (int i = 0; i < tys.size(); ++i) {
        res += ((lam == nullptr || lam->argnames.size() <= i) ? "" : lam->argnames[i]) + " ";
    }
    return res;
}


string
ocamlgen::print_def(string name, const shared_ptr<Code>& code) {
    string res = "";
    if (is_functional(code->type)) {
        // c->argsはとりあえず無視して、ここの引数は出力しないようにしてみる
        res += "let " + name + " " + print_type_from(PURES(TypeFn)(code->type)->from, code->l) + " = ";
        cout << res << endl;
        Compiled v1 = ocamlgen()(code->l->body);
        res += v1.env + v1.body + " in\n";
    } else {
        Compiled cmp = ocamlgen()(code);
        res += cmp.env;
        res += "let " + name + " = " + cmp.body + " in\n";
    }
    return res;
}


Compiled
ocamlgen::operator () (const shared_ptr<Code> &c) {
    cout << *c <<endl << endl;
    if (is_literal(c->lit.val)) {
        assert(c->args.size() == 0);
        return Compiled { c->lit.val, "" };
    } else if (c->lit.val == "_get") {
        Compiled tmp = ocamlgen()(c->args[0]);
        return Compiled { "("+tmp.env+tmp.body+")"+"."+c->args[1]->lit.val, "" };
    } else if (c->lit.val == "type") {
        data_bind[c->args[0]->lit.val] = c->args[1]->rawtype;
        // type_constructors[c->args[0]->lit.val] = c->args[1]->l;
        // data_bind[c->args[0]->lit.val] = parse_data_structure(c->args[1]->l->body);
        // cout << to_string(data_bind[c->args[0]->lit.val]) << endl;

        Compiled res;
        res.env += "type " + ocaml_type_name(c->args[0]->lit.val) + " = { ";
        res.env += print_data_structure(c->args[1]->rawtype);
        res.env += "};;\n";

        // if (MATCHS(TypeProduct)(c->args[1]->rawtype)) {
        //     auto &product = PURES(TypeProduct)(c->args[1]->rawtype);
        //     for (int i = 0; i < product->products.size(); i++) {
        //         res.env += "let "+product->names[i]+" = "
        //     }
        // }

        Compiled v1 = operator()(c->args[2]);
        res.env += v1.env;
        res.body += v1.body;
        return res;
    } else if (c->lit.val == "fuse") {
        Compiled res;
        Guard guard = get_guard(c->args);
        GuardType gtype = get_guard_type(c->args);

        switch (gtype) {
            case GTYPE_COUNTABLE_FINITE: {
                const unsigned fnidx = function_call_counter++;
                int i = 0;
                string matchlist = "";
                for (auto a : guard.countable->l->argnames) {
                    matchlist += a;
                    if (i == guard.countable->l->argnames.size()) {
                        matchlist += ", ";
                    }
                    i++;
                }

                res.body += "(fun " + matchlist + " -> match " + matchlist + " with\n";
                
                for (auto p : guard.finites) {
                    cout << p.first << " = " << *p.second << endl;
                    Compiled v1 = operator()(p.second);
                    res.body += "| " + p.first + " -> " + v1.body + "\n";
                }

                Compiled v1 = operator()(guard.countable->l->body);
                res.body += "| " + matchlist + " -> " + v1.body + "\n";
                res.body += ")\n";
            } break;

            case GTYPE_FINITE: {
            } break;
        }
        return res;
    } else if (builtin_functions.contains(c->lit.val)) {
        Compiled res;
        res.body += c->lit.val;
        // 1 2 3 ) 4 5 6 として、現在のargを逆順にstackしていけば良いことが分かる
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }
        while (!argstack.empty()) {
            Compiled tmp = ocamlgen()(argstack.top());

            res.body += " ";
            argstack.pop();
            res.env += tmp.env;
            res.body += "(" + tmp.body + ")";
        }
        return res;
    } else if (c->l) {
        // bind >
        Compiled res;
        int i = 0;
        // while (!barstack.empty()) {
        //     res.env += print_def(barstack.top(), c->args[i]);
        //     barstack.pop();
        //     i++;
        // }
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }

        vector<string> free;
        for (int i = 0; i < c->l->argnames.size(); i++) {
            if (argstack.empty()) free.push_back(c->l->argnames[i]);
            else {
                cout << c->l->argnames[i] << endl;
                res.env += print_def(c->l->argnames[i], argstack.top());
                argstack.pop();
            }

            // func generator?
//             if (argstack.top()->l && arity(argstack.top()->type) - (int)argstack.top()->l->argnames.size() > 0) {
// //                bind[c->l->argnames[i]] = argstack.top();
//                 cout << c->l->argnames[i] << endl;
//             } else {
//                 cout << c->l->argnames[i] << endl;
//                 res.env += print_def(c->l->argnames[i], argstack.top());
//             }
        }

        // TODO: v1.tmp と tmp の場所
        paircat(res, operator()(c->l->body));

        if (free.size() > 0) {
            string tmp = "fun ";
            for (auto e : free) {
                tmp += e + " ";
            }
            tmp += "-> ";
            res.body = tmp + res.body;
        }
        
        return res;
    } else {
        Compiled res;
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }
        // if (bind.contains(c->lit.val)) {
        //     int pfcounter = pseudo_func_counter;
        //     res.env += "int __pseudo_"+to_string(pfcounter)+"() {\n";
        //     auto cmp = operator()(bind[c->lit.val]);
        //     res.env += cmp.env;
        //     res.env += "return " + cmp.body + ";\n";
        //     res.env += "}\n";
            
        //     res.body += "__pseudo_"+to_string(pfcounter)+"()";
        //     pseudo_func_counter++;
        // } else {
        if (data_bind.contains(c->lit.val)) {
            int i = 0;
            auto records = PURES(TypeProduct)(data_bind[c->lit.val]);
            res.body += "{";
            while (!argstack.empty()) {
                Compiled tmp = ocamlgen()(argstack.top());

                argstack.pop();
                res.body += " ";
//                res.env += tmp.env;
                res.body += records->names[i]+"="+tmp.body+"; ";
                i++;
            }
            res.body += "}";
        } else {
            res.body += c->lit.val;
            while (!argstack.empty()) {
                Compiled tmp = ocamlgen()(argstack.top());

                argstack.pop();
                res.body += " ";
                res.env += tmp.env;
                res.body += "(" + tmp.body + ")";
                // }
            }
        }
        return res;
    }
}
