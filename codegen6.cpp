map<string, shared_ptr<Code>> codegen6::bind;
map<string, shared_ptr<Lambda>> codegen6::type_constructors;
//map<string, DataStructure> codegen6::data_bind;

string
codegen6::compress(const Compiled &&v) {
    return v.body + v.env;
}
void
codegen6::paircat(Compiled &x, const Compiled &&v) {
    x.body += v.body;
    x.env += v.env;
}

string
codegen6::print_type_to(const TypeSignature &ty) {
    if (MATCH(string)(ty)) {
        return PURE(string)(ty);
    } else {
        return print_type_to(get<shared_ptr<TypeSignature>>(ty)->to);
    }
}

string
codegen6::print_type_from(const deque<Type> &tys, const shared_ptr<Lambda> &lam) {
    string res="";
    for (int i = 0; i < tys.size(); ++i) {
        res += print_type_to(tys[i]) + " " + (lam == nullptr ? "" : lam->argnames[i]);
        if (tys.size()-1 != i) {
            res += ", ";
        }
    }
    return res;
}

string
codegen6::print_def(string name, const shared_ptr<Code>& code) {
    string res = "";
    if (code->type.functional()) {
        // c->argsはとりあえず無視して、ここの引数は出力しないようにしてみる
        res += print_type_to(code->type.to) + " " + name + " (" + print_type_from(code->type.from, code->l) + ") {\n";
        Compiled v1 = codegen6()(code->l->body);
        res += v1.env;
        res += "return " + v1.body + ";\n";
        res += "}\n";
    } else {
        Compiled cmp = codegen6()(code);
        res += cmp.env;
        res += print_type_to(code->type.to) + " " + name + " = " + cmp.body + ";\n";
    }
    return res;
}

string
codegen6::print_decl(string name, const TypeSignature &type, bool pointer) {
    string res = "";
    if (type.functional()) {
        res += print_type_to(type.to) + "(*" + name + ")(" + print_type_from(type.from, nullptr) + ");\n";
    } else {
        res += print_type_to(type.to) + " " + name + ";\n";
    }
    return res;
}

DataStructure
codegen6::parse_data_structure(const shared_ptr<Code> &c) {
    DataStructure res;
    if (get_eventual_fnname(c)=="and") {
        AndDS andds;
        for (const auto &arg : c->args) {
            andds.ands.push_back(parse_data_structure(arg));
        }
        res = make_shared<AndDS>(andds);
    }
    else if (get_eventual_fnname(c)=="or") {
        OrDS ords;
        for (const auto &arg : c->args) {
            ords.ors.push_back(parse_data_structure(arg));
        }
        res = make_shared<OrDS>(ords);
    }
    else {
        
        res = make_pair(c->l->argnames[0], c->l->argtypes[0]);
    }
    return res;
}

string
codegen6::print_data_structure(const DataStructure &ds) {
    string res = "";
    if (holds_alternative<shared_ptr<AndDS>>(ds)) {
        for (const auto &eds : get<shared_ptr<AndDS>>(ds)->ands) {
            res += print_data_structure(eds);
        }
    } else if (holds_alternative<shared_ptr<OrDS>>(ds)) {
        for (const auto &eds : get<shared_ptr<OrDS>>(ds)->ors) {
            res += print_data_structure(eds);
        }
    } else {
        string name;
        TypeSignature type;
        tie(name, type) = get<pair<string, TypeSignature>>(ds);
        res += print_decl(name, type);
    }
    return res;
}

Compiled
codegen6::operator () (const shared_ptr<Code> &c) {
    cout << *c <<endl;
    if (is_literal(c->lit.val)) {
        assert(c->args.size() == 0);
        return Compiled { c->lit.val, "" };
    } else if (c->lit.val == "type") {
        type_constructors[c->args[0]->lit.val] = c->args[1]->l;
        data_bind[c->args[0]->lit.val] = parse_data_structure(c->args[1]->l->body);
        cout << to_string(data_bind[c->args[0]->lit.val]) << endl;

        Compiled res;
        res.env += "struct " + c->args[0]->lit.val + "{\n";
        res.env += print_data_structure(data_bind[c->args[0]->lit.val]);
        res.env += "};\n";

        Compiled v1 = operator()(c->args[2]);
        res.env += v1.env;
        res.body += v1.body;
        return res;
    } else if (type_constructors.contains(c->lit.val)
               || builtin_functions.contains(c->lit.val)) {
        Compiled res;
        res.body += c->lit.val + "(";
        // 1 2 3 ) 4 5 6 として、現在のargを逆順にstackしていけば良いことが分かる
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }
        while (!argstack.empty()) {
            Compiled tmp = codegen6()(argstack.top());

            argstack.pop();
            res.env += tmp.env;
            res.body += tmp.body;
            if (argstack.size() != 0) res.body += ", ";
        }
        res.body += ")";
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
        for (int i = 0; i < c->l->argnames.size(); i++) {
            // func generator?
            if (argstack.top()->l && argstack.top()->type.arity() - (int)argstack.top()->l->argnames.size() > 0) {
                bind[c->l->argnames[i]] = argstack.top();
                cout << c->l->argnames[i] << endl;
            } else {
                cout << c->l->argnames[i] << endl;
                res.env += print_def(c->l->argnames[i], argstack.top());
            }
            argstack.pop();
        }
        // TODO: v1.tmp と tmp の場所
        paircat(res, operator()(c->l->body));
        return res;
    } else {
        Compiled res;
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }
        if (bind.contains(c->lit.val)) {
            int pfcounter = pseudo_func_counter;
            res.env += "int __pseudo_"+to_string(pfcounter)+"() {\n";
            auto cmp = operator()(bind[c->lit.val]);
            res.env += cmp.env;
            res.env += "return " + cmp.body + ";\n";
            res.env += "}\n";
            
            res.body += "__pseudo_"+to_string(pfcounter)+"()";
            pseudo_func_counter++;
        } else {
            res.body += c->lit.val + "(";
            while (!argstack.empty()) {
                Compiled tmp = codegen6()(argstack.top());

                argstack.pop();
                res.env += tmp.env;
                res.body += tmp.body;
                if (argstack.size() != 0) res.body += ", ";
            }
            res.body += ")";
        }
        return res;
    }
}
