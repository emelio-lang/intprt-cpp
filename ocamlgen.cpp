map<string, TypeSignature> ocamlgen::type_binds; // TODO: スコープいいの？

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
        res += "let " + name + " " + print_type_from(PURES(TypeFn)(code->type)->from, code->l) + " = \n";
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
    } else if (c->lit.val == "type") {
        type_binds[c->args[1]->lit.val]= c->args[2]->rawtype;
        // type_constructors[c->args[0]->lit.val] = c->args[1]->l;
        // data_bind[c->args[0]->lit.val] = parse_data_structure(c->args[1]->l->body);
        // cout << to_string(data_bind[c->args[0]->lit.val]) << endl;

        Compiled res;
        // res.env += "struct " + c->args[0]->lit.val + "{\n";
        // res.env += print_data_structure(data_bind[c->args[0]->lit.val]);
        // res.env += "};\n";

        Compiled v1 = operator()(c->args[2]);
        res.env += v1.env;
        res.body += v1.body;
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
        res.body += c->lit.val;
        while (!argstack.empty()) {
            Compiled tmp = ocamlgen()(argstack.top());

            argstack.pop();
            res.body += " ";
            res.env += tmp.env;
            res.body += "(" + tmp.body + ")";
            // }
        }
        return res;
    }
}
