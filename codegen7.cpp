map<string, TypeSignature> codegen7::data_bind; // TODO: スコープいいの？
unsigned int codegen7::type_count = 0;
unsigned int codegen7::dummy_count = 0;
map<string, string> codegen7::fuse_bind;
pseudo_map<TypeSignature, string> codegen7::tmptypes;
pseudo_map<deque<TypeSignature>, string> codegen7::type_constrs;
map<string, vector<string>> codegen7::spvals;


// TODO: 名前をつける関数のトップレベルに引数が全てない場合ダメになる
// (|foo| ...) ((|a b| ...))みたいなやつとか

string
codegen7::compress(const Compiled &&v) {
    return v.body + v.env;
}
string
codegen7::compress(const Compiled &v) {
    return v.body + v.env;
}
void
codegen7::paircat(Compiled &x, const Compiled &&v) {
    x.body += v.body;
    x.env += v.env;
}
void
codegen7::paircat(Compiled &x, const Compiled &v) {
    x.body += v.body;
    x.env += v.env;
}
void codegen7::paircat(tuple<string,string,string> &x, const tuple<string,string,string> &&y) {
    get<0>(x) += get<0>(y);
    get<1>(x) += get<1>(y);
    get<2>(x) += get<2>(y);
}

string
codegen7::c_type_name(string s, bool bPrefix) {
    string prefix = "";
    if (bPrefix) {
        if (MATCHS(TypeSum)(data_bind[s])) prefix = "struct ";
        if (MATCHS(TypeProduct)(data_bind[s])) prefix = "struct ";
    }
    return prefix + "t_" + tolower(s);
}

string
codegen7::typesig_to_typename(const TypeSignature &type) {
    if (tmptypes.contains(type)) { return tmptypes[type]; }
    else if (defined_polymos.contains(type)) { return defined_polymos[type]; }
    else if (MATCH(string)(type)) {
        return c_type_name(PURE(string)(type));
    } else {
        ASSERT(false, "おかしいね");
    }
}

string
codegen7::typesig_to_constructor(const TypeSignature &type) {
    if (MATCH(string)(type)) {
        return PURE(string)(type);
    } else if (MATCHS(Parametered)(type)) {
        return PURES(Parametered)(type)->type;
    } else {
        ASSERT(false, "おかしいね");
    }
}

string
codegen7::new_tmptype(const TypeSignature type) {
    type_count++;
    tmptypes[type] = "t_"+to_string(type_count);
    return "t_"+to_string(type_count);
}

string
codegen7::new_dummy_var() {
    dummy_count++;
    return "__x"+to_string(dummy_count);
}

bool
codegen7::is_spval(string a) {
    for (auto &[_, e]: spvals) {
        for (auto &i : e) {
            if (a == i) return true;
        }
    }
    return false;
}

int
codegen7::spval_to_int(SpecialValue sp) {
    for (auto &[_, e]: spvals) {
        int j = 0;
        for (auto &i : e) {
            if (sp.val == i) return j;
            j++;
        }
    }
    return 0;
}

string
codegen7::print_polymos() {
    string res = "";
    
    for (auto type : defined_polymos.get_keys()) {
        assert(MATCHS(Parametered)(type));
        auto name = defined_polymos[type];
        auto param = PURES(Parametered)(type);
        map<string,TypeSignature> _params;
        for (int i = 0; i < polymos[name].size(); i++) {
            _params[polymos[name][i]] = param->params[i];
        }
        auto tmp = print_data_structure(data_bind[param->type], "", _params);
        res += get<0>(tmp)+get<1>(tmp)+get<2>(tmp) + "\n";
    }
    return res;
}

// NOTE: １つ目と２つ目を名前で挟む
tuple<string,string,string>
codegen7::print_data_structure(const TypeSignature type, string type_name, const map<string,TypeSignature> &params) {
    tuple<string,string,string> res = make_tuple("","","");
    bool bNewTmptype = type_name == "";

    if (tmptypes.contains(type)) {
        get<1>(res) += tmptypes[type] + " ";
        return res;
    }

    if (MATCHS(TypeProduct)(type)) {
        const auto prod = PURES(TypeProduct)(type);
        const string tmptype = bNewTmptype ? new_tmptype(type) : type_name;
        get<0>(res) += "struct " + tmptype + "{\n";
        for (int i = 0; i < prod->products.size(); i++) {
            auto tmp = print_data_structure(prod->products[i]);
            if (prod->names.size() == 0) {
                get<0>(res) += get<0>(tmp) + get<1>(tmp) + " e" + to_string(i) + get<2>(tmp) + ";\n";
            } else {
                get<0>(res) += get<0>(tmp) + get<1>(tmp) + prod->names[i] + get<2>(tmp) + ";\n";
            }
        }
        get<0>(res) += "};\n";
        get<1>(res) += "struct " + tmptype + " ";
    } else if (MATCHS(TypeSum)(type)) {
        const auto sum = PURES(TypeSum)(type);
        const string tmptype = bNewTmptype ? new_tmptype(type) : type_name;
        get<0>(res) += "struct "+tmptype+" {\n";
        get<0>(res) += "unsigned short rtti;\n";
        get<0>(res) += "union {\n";
        
        // get<0>(res) += "enum {";
        for (int i = 0, len=sum->sums.size(); i < len; i++) if (MATCH(SpecialValue)(sum->sums[i])) {
                if (spvals.contains(tmptype)) spvals[tmptype] = {};
                spvals[tmptype].emplace_back(PURE(SpecialValue)(sum->sums[i]).val);
                // get<0>(res) += PURE(SpecialValue)(sum->sums[i]).val + (i!=len-1?", ":"");
            }
        // get<0>(res) += "} sp;\n";
        get<0>(res) += "int sp;\n";

        for (int i = 0; i < sum->sums.size(); i++) {
            if (MATCH(SpecialValue)(sum->sums[i])) continue;
            auto tmp = print_data_structure(sum->sums[i]);
            get<0>(res) += get<0>(tmp) + get<1>(tmp) + " e" + to_string(i) + get<2>(tmp) + ";\n";
        }
        get<0>(res) += "} uni;\n";
        get<0>(res) += "};\n";
        get<1>(res) += "struct " + tmptype + " ";
    } else if (MATCHS(TypeFn)(type)) {
        // to (*name) (args, ...);
        const auto fn = PURES(TypeFn)(type);
        paircat(res, print_data_structure(fn->to));
        get<1>(res) += "(*";
        get<2>(res) += ")(";
        for (int i = 0; i < fn->from.size(); ++i) {
            auto tmp = print_data_structure(fn->from[i]);
            get<0>(res) += get<0>(tmp);
            get<2>(res) += get<1>(tmp)+get<2>(tmp);
            if (i != fn->from.size()-1) get<2>(res) += ",";
        }
        get<2>(res) += ")";
    } else if (MATCH(string)(type)) {
        auto str = PURE(string)(type);
        // if (params.contains(str)) {
        //     return print_data_structure(params[str], params);
        // } else {
            get<1>(res) = (str == "int" ? str : c_type_name(str) + "*") + " ";
        // }
    }
    else if (MATCHS(Parametered)(type)) {
        auto param = PURES(Parametered)(type);
        if (!defined_polymos.contains(type)) {
            defined_polymos[type] = "t_"+param->type+to_string(defined_polymos.size());
        }
        get<1>(res) = defined_polymos[type] + "* ";
    }
    // 特殊地はintとして扱われる
    else if (MATCH(SpecialValue)(type)) { return make_tuple("", "int ", ""); }
    return res;
}

// NOTE: print_defからしか呼んじゃダメ。ダミー変数を作ってargstackを操作します
// 例えば、(|foo| ... ) ((|a b| ...) 3)<-ここ みたいな式について'<-ここ'の所の関数をfooと名付けるわけですが、この時
// この内側にコンパイラを渡す際に、ダミー変数としてかぶらない名前（今は__x1みたいにしてます）をargstackに最初から突っ込んでおくことで、
// 最終的にfooを呼ぶ時（引数は２つ渡すべき）の挙動を再現してコンパイルできます
string
codegen7::print_type_from(const deque<TypeSignature> &tys, const shared_ptr<Lambda> &lam) {
    string res="";
    for (int i = 0; i < tys.size(); ++i) {
        auto tmp = print_data_structure(tys[i]);
        res += get<0>(tmp);
        if (lam->argnames.size() > i) {
            res += get<1>(tmp) + (lam == nullptr ? "" : lam->argnames[i]) + get<2>(tmp);
        } else {
            res += get<1>(tmp) + new_dummy_var() + get<2>(tmp);
            Code c = { nullptr, "__x"+to_string(dummy_count), {} };
            deep_copy_from(c.type,tys[i]);
            dummy_argstack.push(make_shared<Code>(c));
        }

        if (tys.size()-1 != i) {
            res += ", ";
        }
    }
    return res;
}

inline Compiled
codegen7::print_decl(string name, const TypeSignature &typesig) {
    Compiled res;
    auto tmp = print_data_structure(typesig);
    res.global += get<0>(tmp);
    res.body += get<1>(tmp) + " " + name + get<2>(tmp);
    return res;
}

// TODO: global捨ててない？
Compiled
codegen7::define(string name, const shared_ptr<Code>& code) {
    Compiled res;
    if (MATCHS(TypeFn)(code->type)) {
        auto fn = PURES(TypeFn)(code->type);
            // c->argsはとりあえず無視して、ここの引数は出力しないようにしてみる
        auto tmp = print_data_structure(code->type);
        res.body += get<0>(tmp);

        const string type_to = get<1>(tmp).substr(0, get<1>(tmp).length()-2);
        const string prottype = type_to + name + "(" + print_type_from(fn->from, code->l) + ") ";
//        res.body += prottype + ";\n";
        res.body += prottype + " {\n";
        
        Compiled v1 = codegen7(dummy_argstack)(code->l->body);
        res.global += v1.global;
        res.body += v1.env;
        if (v1.return_required) res.body += "return " + v1.body + ";\n";
        else res.body += v1.body;
        res.body += "}\n";
    // } else if (MATCHS(TypeSum)(code->type) && MATCHS(TypeFn)(PURES.BODY(TypeSum)(code->type)->sums[0])) {
    //     // fuseなら何もしない
    //     Compiled v1 = codegen7()(code->l->body);
    //     res.body += v1.env;
    //     res.body += v1.body;
    //     fuse_bind[name] = "__fuse" + to_string(fuse_bind.size());
    } else {
        auto tmp = print_data_structure(code->type);
        res.env += get<0>(tmp);

        Compiled cmp = codegen7()(code);
        res.global += cmp.global;
        res.env += cmp.env;
        res.body += get<1>(tmp) + name + " = " + cmp.body + ";\n";
    }
    return res;
}

Compiled codegen7::fuse(const shared_ptr<Code> &c) {
    Compiled res;
    res.return_required = false;
    int ari = arity(c->args[0]->type);
    deque<string> dummies;
    stack<shared_ptr<Code>> argstack_bkp = argstack;
    for (int i = 0; i < ari; i++) {
        dummies.emplace_back(argstack.top()->lit.val);
//            argstack.pop();
    }

    cout << "yahoo " << to_string(PURES(TypeFn)(c->type)->from[0]) << endl;

    // 1個めの引数がわけられているかどうか
    if (MATCHS(TypeSum)(PURES(TypeFn)(c->type)->from[0])) {
        auto sum = PURES(TypeSum)(PURES(TypeFn)(c->type)->from[0])->sums;
        const string name = argstack.top()->lit.val;
        for (int i = 0; i < c->args.size(); i++) {
            int rtti = distance(sum.begin(), find(sum.begin(), sum.end(), PURES(TypeFn)(c->args[i]->type)->from[0]));
            bool bSpecialValue = MATCH(SpecialValue)(PURES(TypeFn)(c->args[i]->type)->from[0]);
            string downcast = bSpecialValue ? name+"->uni.sp" : name+"->uni.e"+to_string(rtti);
            argstack.top()->type = sum[rtti];
            argstack.top()->lit.val = downcast;

            if (is_literal(c->args[i]->lit.val)) {
                res.body += "if ("+dummies[0]+"->rtti == "+to_string(rtti)+" && "+c->args[i]->lit.val+" == "+downcast+") {\n";
            } else if (bSpecialValue) {
                res.body += "if ("+dummies[0]+"->rtti == "+to_string(rtti)+" && "+to_string(spval_to_int(PURE(SpecialValue)(PURES(TypeFn)(c->args[i]->type)->from[0]))) + "/*"+PURE(SpecialValue)(PURES(TypeFn)(c->args[i]->type)->from[0]).val+"*/ == "+name+"->uni.sp) {\n";
            } else {
                res.body += "if ("+dummies[0]+"->rtti == "+to_string(rtti)+") {\n";
            }
            Compiled v1 = operator()(c->args[i]); argstack = argstack_bkp;
            res.global += v1.global;
            res.body += v1.env;
            if (v1.return_required) res.body += "return " + v1.body + ";\n";
            else res.body += v1.body;
            res.body += "}\n";
        }
    } else {
        res.body += "if (false) {\n";
        for (int i = 0; i < c->args.size(); i++) {
            if (is_literal(c->args[i]->l->argnames[0])) {
                res.body += "} else if ("+c->args[i]->l->argnames[0]+" == "+dummies[0]+") {\n";
            } else {
                res.body += "} else {\n";
            }
            Compiled v1 = operator()(c->args[i]); argstack = argstack_bkp;
            res.global += v1.global;
            res.body += v1.env;
            if (v1.return_required) res.body += "return " + v1.body + ";\n";
            else res.body += v1.body;
        }
        res.body += "}";
    }
    return res;
}

// body空白で返します, env,globalを使用しています
Compiled
codegen7::print_constructor(string type_name, const TypeSignature& newtype, int rtti/*=-1*/) {
    Compiled res;
    TypeSignature norm_newtype = normalized(newtype);

    cout << "heyy" << to_string(norm_newtype) << endl;
    
    // コンストラクタを出力
    if (MATCHS(TypeSum)(norm_newtype)) {
        auto sum = PURES(TypeSum)(norm_newtype);
        for (int i = 0; i < sum->sums.size(); i++) {
            paircat(res, print_constructor(type_name, sum->sums[i], i));
        }

//        if (sum->special_values.size() != 0) {
            res.env += c_type_name(type_name)+"* "+type_name+"(int in) {\n";
            res.env += c_type_name(type_name) + " *tmp = ("+c_type_name(type_name)+"*)malloc(sizeof("+c_type_name(type_name,false)+"));\n";
            res.env += "tmp->uni.sp = in;\n";
            res.env += "tmp->rtti = "+to_string(sum->sums.size())+";\n";
            res.env += "return tmp;\n";
            res.env += "}\n";
//        }

    } else {
        if (MATCHS(TypeProduct)(norm_newtype)) {
            auto prod = PURES(TypeProduct)(norm_newtype);
            const string constr_name = type_name+to_string(type_constrs.size());
            type_constrs[prod->products] = constr_name;
            res.env += c_type_name(type_name) + "* " + constr_name + "(";
            for (int i = 0; i < prod->products.size(); i++) {
                Compiled printed = print_decl(prod->names[i], prod->products[i]);
                res.global += printed.global;
                res.env += compress(printed);
                if (i != prod->products.size()-1) {
                    res.env += ", ";
                }
            }
            res.env += ") {\n";
            res.env += c_type_name(type_name) + " *tmp = ("+c_type_name(type_name)+"*)malloc(sizeof("+c_type_name(type_name,false)+"));\n";
            if (rtti >= 0) res.env += "tmp->rtti = "+to_string(rtti)+";\n";
            for (int i = 0; i < prod->names.size(); i++) {
                res.env += "tmp->"+(rtti>=0 ? "uni.e"+to_string(rtti)+"." : "")+prod->names[i]+" = "+prod->names[i]+";\n";
            }
            res.env += "return tmp;\n";
            res.env += "}\n";
        } else {
            // TODO: string, Parametered(これはpolyで実装するはず), 
        }
    }
    
    return res;
}

Compiled codegen7::type_def(const string newtype_name, const TypeSignature &newtype) {
    // const auto newtype_name = c->args[0]->lit.val;
    // const auto newtype = c->args[1]->rawtype;

    // if (c->args.size() == 4) {
    //     polymos[newtype_name] = deque<string>(c->args[2]->l->argnames.begin(), c->args[2]->l->argnames.end());
    // }
        
    data_bind[newtype_name] = newtype;

    // 型定義を出力
    Compiled res;
    auto tmp = print_data_structure(newtype, c_type_name(newtype_name,false));
    res.env += get<0>(tmp) + ";\n";


    paircat(res, print_constructor(newtype_name, newtype));
    // if (MATCHS(TypeSum)(newtype)) {
    //     auto sum = PURES(TypeSum)(newtype);
    //     for (int i = 0; i < sum->sums.size(); i++) {
    //         if (MATCHS(Parametered)(sum->sums[i])) {
    //             auto param = PURES(Parametered)(sum->sums[i]);
                
    //         } else {
    //             res.env += c_type_name(newtype_name) + "* " + newtype_name + "(" + tmptypes[sum->sums[i]] + "* in) {\n";
    //             // Sumを構成している型はすべてtmptypeとして実装されているはず...？
    //             res.env += newtype_name + " *tmp = ("+c_type_name(newtype_name)+"*)malloc(sizeof("+c_type_name(newtype_name)+"));\n";
    //             res.env += "tmp->uni.e"+to_string(i)+" = in;\n";
    //             res.env += "tmp->rtti = "+to_string(i)+";\n";
    //             res.env += "return tmp;\n";
    //             res.env += "}\n";
    //         }
    //     }
    //     if (sum->special_values.size() != 0) {
    //         res.env += c_type_name(newtype_name)+"* "+newtype_name+"(int in) {\n";
    //         res.env += newtype_name + " *tmp = ("+c_type_name(newtype_name)+"*)malloc(sizeof("+c_type_name(newtype_name)+"));\n";
    //         res.env += "tmp->uni.sp = in;\n";
    //         res.env += "tmp->rtti = "+to_string(sum->sums.size())+";\n";
    //         res.env += "return tmp;\n";
    //         res.env += "}\n";
    //     }
    // }
    // if (MATCHS(TypeProduct)(newtype)) {
    //     auto prod = PURES(TypeProduct)(newtype);
    //     for (int i = 0; i < prod->products.size(); i++) {
    //         Compiled printed = print_decl(prod->names[i], prod->products[i]);
    //         res.global += printed.global;
    //         res.env += compress(printed);
    //         if (i != prod->products.size()-1) {
    //             res.env += ", ";
    //         }
    //     }
    //     res.env += ") {\n";
    //     res.env += newtype_name + " *tmp = ("+c_type_name(newtype_name)+"*)malloc(sizeof("+c_type_name(newtype_name)+"));\n";
    //     for (int i = 0; i < prod->names.size(); i++) {
    //         res.env += "tmp->"+prod->names[i]+" = "+prod->names[i]+";\n";
    //     }
    //     res.env += "return tmp;\n";
    //     res.env += "}\n";
    // }

    return res;
}

Compiled
codegen7::operator () (const shared_ptr<Code> &c) {
    cout << *c <<endl << endl;
    if (is_literal(c->lit.val)) {
        assert(c->args.size() == 0);
        return Compiled { c->lit.val, "" };
       
    } else if (c->lit.val == "_get") {
        Compiled tmp = codegen7()(c->args[0]);
        return Compiled { "("+tmp.env+tmp.body+")"+"->"+c->args[1]->lit.val, "", tmp.global };
    } else if (c->lit.val == "type") {
        Compiled res;
        paircat(res, type_def(c->args[0]->lit.val, c->args[1]->rawtype));
        paircat(res, operator()(c->args[2]));
        return res;
    } else if (c->lit.val == "fuse") {
        return fuse(c);

    } else if (builtin_functions.contains(c->lit.val)) {
        Compiled res;
        const map<string,string> sym = { { "add", "+" }, {"sub","-"}, {"mul","*"}, {"div","/"} };
        res.body += "(";
        // 1 2 3 ) 4 5 6 として、現在のargを逆順にstackしていけば良いことが分かる
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }
        while (!argstack.empty()) {
            Compiled tmp = codegen7()(argstack.top());

            argstack.pop();
            res.global += tmp.global;
            res.env += tmp.env;
            res.body += tmp.body;
            if (argstack.size() != 0) res.body += sym.at(c->lit.val);
        }
        res.body += ")";
        return res;
    } else if (c->l) {
        Compiled res;
        int i = 0;
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }

        // vector<string> free; NOTE: つまりocamlgenでは使っていたこれがいらない
        for (int i = 0; i < c->l->argnames.size(); i++) {
            // NOTE: 上に書いたようにダミー変数を使ってコンパイルするため、ココでargstackが空であることは無いはずです
            assert(!argstack.empty());

            if (!is_literal(c->l->argnames[i])) {
                auto tmp = define(c->l->argnames[i], argstack.top());
                res.global += tmp.global;
                res.env += compress(tmp);
            }
            argstack.pop();
            // func generator?
        }

        // TODO: v1.tmp と tmp の場所
        Compiled tmp = operator()(c->l->body);
        paircat(res, tmp);
        res.return_required = tmp.return_required;
        res.global += tmp.global;

        for (int i = 0; i < c->l->argnames.size(); i++) {
            if (!is_literal(c->l->argnames[i]) && arity(c->l->argtypes[i])==0)
                res.env += "delete "+c->l->argnames[i]+";\n";
        }
        
        return res;
    } else if (MATCH(SpecialValue)(c->type)) {
        return Compiled { to_string(spval_to_int(PURE(SpecialValue)(c->type)))+"/*"+PURE(SpecialValue)(c->type).val+"*/", "" };
    } else {
        Compiled res;
        for (int i = c->args.size()-1; i >= 0; i--) {
            argstack.push(c->args[i]);
        }

        if (c->cRawtype) {
            int i = 0;
//            res.body += typesig_to_constructor(c->rawtype) + "(";
//            cout << "heyyyooo" << to_string(c->type) << endl;
            deque<TypeSignature> idx;
            for (auto e : c->args) { idx.emplace_back(e->type); }
            res.body += (type_constrs.contains(idx) ? type_constrs[idx] :typesig_to_constructor(c->rawtype))  + "(";
            while (!argstack.empty()) {
                // Compiled tmp = codegen7()(argstack.top());
                auto tmp = define("tmp"+to_string(i), argstack.top());
                res.global += tmp.global;
                res.env += compress(tmp);
                
                argstack.pop();
                res.global += tmp.global;
//                res.env += tmp.env;
                res.body += /*records->names[i]+"="+*/"tmp"+to_string(i)+", ";
                i++;
            }
            
            if (res.body.size() >= 2) res.body = res.body.substr(0, res.body.length()-2);
            res.body += ")";
        } else {
            string destruct_buffer = "";
            if (fuse_bind.contains(c->lit.val)) {
                res.body += fuse_bind[c->lit.val];
            } else {
                res.body += c->lit.val;
            }
            if (argstack.size() != 0) {
                int i = 0;
                res.body += "(";
                while (!argstack.empty()) {
//                    Compiled tmp = codegen7()(argstack.top());
                    // NOTE: 引数を用意する時にポインタを確保しておいて、あとでデストラクタを呼ぶ
                    auto tmp = define("tmp"+to_string(i), argstack.top());
                    res.global += tmp.global;
                    res.env += compress(tmp);
                    if (MATCH(string)(argstack.top()->type) && PURE(string)(argstack.top()->type) != "int") {
                        destruct_buffer += "delete "s + "tmp"+to_string(i) + ";\n";
                    }

                    argstack.pop();
                    // res.global += tmp.global;
                    // res.env += tmp.env;
                    // res.body += tmp.body;
                    res.body += "tmp"+to_string(i)+", ";
                    // }
                    i++;
                }
                if (res.body.size() >= 2) res.body = res.body.substr(0, res.body.length()-2);
                res.body += ")";
                res.body = destruct_buffer;
            }
        }

        
        
        return res;
    }
}
