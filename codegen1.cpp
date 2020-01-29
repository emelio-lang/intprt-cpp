
pair<string,string>
codegen::operator () (const shared_ptr<Code> c) {

    string res = "";
    string env = "";

    for (int i = c->args.size()-1; i >= 0; i--) {
        argstack.push_back(c->args[i]);
    }

    if (c->l) {
        // instant function call

        auto _argstack = this->argstack;
        for (auto argname : c->l->argnames) {
            if (_argstack.empty()) {
                // unbinded.push_back(argname);
                return make_pair("", "");
            } else {
                cout << argname << ":" << *_argstack.back() << endl << endl;

                if (_argstack.back()->arity == 0 && is_root) {
                    outprg(env, argname + ":");
                    is_root = true;
                    this->argstack.clear();
                    outprg(env, env, this->operator()(_argstack.back()));
                    outprg(env, "ret\n");
                    
                    bind[argname] = shared_ptr<Code>();
                } else {
                    bind[argname] = _argstack.back();
                }
                
                _argstack.pop_back();
            }
            // defined.insert(argname);
        }
        this->argstack = _argstack;
        
        is_root = true;
        outprg(res, env, this->operator()(c->l->body));

        return make_pair(res, env);
    }
    else if (c->lit.val == "fuse") {
    }
    else if (builtin_functions.contains(c->lit.val)) {
        // builtin-function call

        if (bf2arity.at(c->lit.val) != argstack.size()) return make_pair("", "");

        cout << "buitin-function call" << endl;
        cout << *c <<endl;
        cout << "current stack: ";
        SCOUT(argstack);
        
        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            is_root = false;
            outprg(res, env, this->operator()(arg));
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();
        
        outprg(res, c->lit.val);
    }
    else if (is_literal(c->lit.val)) {
        // literal

        cout << "literal" << endl;
        cout << *c <<endl;
        
        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            is_root = false;
            outprg(res, env, this->operator()(arg));
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();


        outprg(res, "push "+c->lit.val);
    }
    else {
        // general function call

        // NOTE: spill counter cuz it's grobval
        const unsigned fnidx = function_call_counter++;
        const unsigned argnum = argstack.size();
        const bool is_recursive = !bind.contains(c->lit.val);

        cout << "general function call: " << c->lit.val << endl;
        cout << "current stack: ";
        SCOUT(argstack);

        if (!bind[c->lit.val].get()) {
            outprg(res, "call " + c->lit.val);
        }
        else if (bind[c->lit.val]->arity - argnum > 0) {
            cout << "few !!" << endl;
            // copy(argstack.begin(), argstack.end(), back_inserter(bind[c->lit.val]->args));
            // outprg(res, env, this->operator()(c->l->body));
            return make_pair("","");
        }
        else {
            outprg(res, ";; " + c->lit.val);

            //body call
            is_root = false;
            outprg(res, env, this->operator()(bind[c->lit.val]));
        
            argstack.clear();
        }
    }


    return make_pair(res, env);
}
