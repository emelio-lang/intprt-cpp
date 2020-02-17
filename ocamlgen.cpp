string
ocamlgen::operator () (const shared_ptr<Code> c) {
    string body;

    if (c->l) {
        body += "(fun -> ";

        // (fun argnames -> body) (arg1) (arg2);
        // let argname1 = arg1;
        // let argname2 = arg2;
        // ...;
        
            

        if (c->l->argnames.size() != 0) {

            for (auto argname : c->l->argnames) {
                body += "let "+argname+" = ";
                body += argname + " ";
            }

            body += " ";
        }

        body += "(";

        body += this->operator()(c->l->body);
        
        body += ")";
        
        for (auto arg : c->args) {
            body += this->operator()(arg) + " ";
        }
        
        body += ")";
    } else if (c->lit.val == "fuse") {
        // NOTE: only see direct arguments
        Guard guard = get_guard(c->args);
        GuardType gtype = get_guard_type(c->args);

        switch (gtype) {
            case GTYPE_COUNTABLE_FINITE: {
                body += "(";
                body += "fun x -> ";

                for (pair<string, shared_ptr<Code>> fnt : guard.finites) {
                    body += "(";
                    body += "if x == " + fnt.first + " then ";
                    body += this->operator()(fnt.second);
                    body += "else ";
                }

                body += this->operator()(guard.countable->l->body);

                for (int i=0; i<guard.finites.size(); i++) {
                    body += ")";
                }
                
                body += ")";
            } break;
                
            case GTYPE_FINITE: {
            } break;
        }
        
    } else {
        // TODO: ocaml 再帰で積んでる
        body += "(";
        const map<string,string> builtins = {
            { "add", "(+)" },
            { "sub", "(-)" },
            { "div", "(/)" },
            { "mul", "(*)" },
            { "negate", "-" },
            { "concat", "(+)" },
        };
        
        if (builtins.contains(c->lit.val)) {
            body += builtins.at(c->lit.val) + " ";
        }
        else body += c->lit.val + " ";
        for (auto arg : c->args) {
            body += this->operator()(arg) + " ";
        }
        body += ")";
    }
    
    return body;
}
