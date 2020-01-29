
StackLanguage
codegen2::operator () (const shared_ptr<Code> c) {

    StackLanguage res {}; 

    for (int i = c->args.size()-1; i >= 0; i--) {
        argstack.push_back(c->args[i]);
    }

    if (c->l) {
        // instant function call

        auto _argstack = this->argstack;
        unsigned kyomu = 0;
        for (auto argname : c->l->argnames) {
            if (_argstack.empty()) {
                cout << argname << endl;
                unbinded.push_back(argname);
                kyomu++;
            } else {
                cout << argname << ":" << *_argstack.back() << endl << endl;

                outprg(res.env, ":" + argname);
                is_root = true;
                this->argstack.clear();
                outenv(res, this->operator()(_argstack.back()));
                if (_argstack.back()->arity != 0) {
                    outprg(res.env, "decbsp"+to_string(_argstack.back()->arity));
                    outprg(res.env, "rewind"+to_string(_argstack.back()->arity));
                }
                outprg(res.env, ";");
                
                bind[argname] = _argstack.back();
                _argstack.pop_back();
            }
            defined.insert(argname);
        }
        this->argstack = _argstack;

//        reverse(unbinded.end()-kyomu, unbinded.end());
        
        is_root = true;
        outroot(res, this->operator()(c->l->body));

        return res;
    }
    else if (c->lit.val == "fuse") {
        // fuse

        // NOTE: only see direct arguments
        Guard guard = get_guard(c->args);
        GuardType gtype = get_guard_type(c->args);

        switch (gtype) {
            case GTYPE_COUNTABLE_FINITE: {
                // fuseのargumentは関係ないので消しておく
                // TODO: これだと、最初のstackに詰める処理が無駄...
                for (int i = 0; i < c->args.size(); i++)
                    argstack.pop_back();

                // for (auto a : argstack) {
                //     outprg(res, ":)");
                //     stringstream ss;
                //     ss << *a;
                //     outprg(res, ss.str());
                // }

                // TODO: ここでもしargstackが空であれば、スタックから読み込むべき

                // run first arg to patternmatch
                // TODO: 関数の時は？
                // {
                //     auto _argstack = this->argstack;
                //     this->argstack.clear();
                //     outprg(res, env, this->operator()(_argstack.back()));
                //     this->argstack = _argstack;
                // }
                outprg(res.root, "dup1");

                
                for (pair<string, shared_ptr<Code>> fnt : guard.finites) {
                    outprg(res.root, fnt.first);
                    outprg(res.root, "==");
                    outprg(res.root, "if");
                    outprg(res.root, "drop");
                    {
                        auto _argstack = this->argstack;
                        this->argstack.clear();
                        outroot(res, this->operator()(fnt.second));
                        this->argstack = _argstack;
                    }
                    outprg(res.root, "else");

                    conditional_counter++;
                }

//                outprg(res.root, "call __pop");
                outprg(res.root, "pop");
                // NOTE: これより内部の引数はcodegen内で実行されるので皮だけでok
                for (auto argname : guard.countable->l->argnames)
                    defined.insert(argname);
                outroot(res, this->operator()(guard.countable));
                
                for (int w = 0; w < guard.finites.size(); ++w) {
                    outprg(res.root, "then");
//                    outprg(res.root, ";");
                }
            } break;
                
            case GTYPE_FINITE: {
            } break;
        }

        argstack.clear();
    }
    else if (builtin_functions.contains(c->lit.val)) {
        // builtin-function call

        if (bf2arity.at(c->lit.val) != argstack.size())
            return res;

        cout << "buitin-function call" << endl;
        cout << *c <<endl;
        cout << "current stack: ";
        SCOUT(argstack);
        
        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            is_root = false;
            outroot(res, this->operator()(arg));
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();

        const map<string,string> forth_builtin = {
            { "add", "+" },
            { "negate", "negate" },
            { "sub", "-" },
            { "mul", "*" },
            { "div", "/" },
        };
        
        outprg(res.root, forth_builtin.at(c->lit.val));
    }
    else if (is_literal(c->lit.val)) {
        // literal

        cout << "literal" << endl;
        cout << *c <<endl;
        
        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            is_root = false;
            outroot(res, this->operator()(arg));
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();

        outprg(res.root, c->lit.val);
    }
    else {
        // general function call

        // NOTE: spill counter cuz it's grobval
        const unsigned fnidx = function_call_counter++;
        const unsigned argnum = argstack.size();
        const bool is_recursive = !defined.contains(c->lit.val);

        cout << "general function call: " << c->lit.val << endl;
        cout << "current stack: ";
        SCOUT(argstack);
        
        auto _argstack = argstack;
        for (auto arg : _argstack) {
            argstack = {};
            is_root = false;
            if (arg->arity == 0) {
                outroot(res, this->operator()(arg));
            } else {
                this->insfunc_counter++;
                outprg(res.root, "'F"+to_string(this->insfunc_counter));
                outprg(res.env, ": F"+to_string(this->insfunc_counter));
                outenv(res, this->operator()(arg));
//                outprg(res.env, "decbsp"+to_string(arg->l->argnames.size()));
                outprg(res.env, ";");
            }
            // copy(argstack.begin(), argstack.end(), back_inserter(_argstack));
        }
        argstack.clear();

        // call actual function
        if (find(unbinded.begin(), unbinded.end(), c->lit.val) != unbinded.end()) {
            // if it's unbinded arg, let it get from fstack
            int dist = abs(distance(unbinded.begin(), find(unbinded.begin(), unbinded.end(), c->lit.val))) + 1;
            
            if (c->args.size() == 0) {
                outprg(res.root, "dup" + to_string(dist));
            } else {
                outprg(res.root, "!dup" + to_string(dist));
            }
        } else {
            outprg(res.root, c->lit.val);
        }
    }


    return res;
}
