/* ========================================================================
   $File: notation.cpp $
   $Date: Sep 09 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"
#include "notation.h"

// 変数の名前かどうか
inline bool is_notation_variable(const string& s) {
    bool res = true;
    for (char c : s) if (!isupper(c)) res = false;
    return res;
}

// 自由変数の名前かどうか
inline bool is_notation_free_variable(const string& s) {
    bool res = true;
    for (int i = 0; i < s.size()-1; ++i)
        if (!isupper(s[i])) res = false;
    if (s.back() != 's') res = false;
    return res;
}


// 自由変数が最初にある時以外、定義が(λ <add>)[(λ <4>)[],(λ <8>)[],]として解釈しなければならないところが、
// [(λ <add>)[],(λ <4>)[],(λ <8>)[],]として流れてくるのを修正する
void fix_defs_fv(map<string, shared_ptr<Code>> &defs) {
    for (auto& [key, val] : defs) {
        if (is_notation_free_variable(key) && val->lit.val == "" && !val->l) {
            shared_ptr<Code> newcode = make_shared<Code>(*val->args[0]);
            copy(val->args.begin()+1,
                 val->args.end(),
                 back_inserter(newcode->args));
            val = newcode;
        }
    }
}

// 変数名で記述されたコード c に定義セット d を代入していく
void replace_code(shared_ptr<Code> c, const map<string, shared_ptr<Code>> &d) {
    if (c->l) {
        for (string &a : c->l->argnames) {
            if (CONTAINS(d, a)) {
                /* 引数名も変える(おそらく、ただの変数として定義されているはずなのでそのままlitを引っ張り出す)

                   ex.
                   (|A| ... ) { A => (λ <windows>)[] }
                   を
                   (|windows| ...)
                   に変換
                */
                a = d.at(a)->lit.val; 
            }
        }

        replace_code(c->l->body, d);
    } else if (c->lit.val != "") {
        if (CONTAINS(d, c->lit.val) && (is_notation_variable(c->lit.val) || is_notation_free_variable(c->lit.val))) {
            /*
              変数の書換
              ・引数がない場合はそのまま置き換える
              ・引数があれば（つまり、Code { lit = 変数名, args = 存在, l = null }になっているはず)
              　lに新しくLambdaを作って、そのbodyとして定義を代入する
            */
            if (c->args.size() == 0) {
                *c = *d.at(c->lit.val);
                return;
            } else {
                if (c->l) cout << "[GIWAKU] どうやってやったんだ..." << endl;

                c->l = shared_ptr<Lambda>(new Lambda);
                c->l->body = make_shared<Code>(*d.at(c->lit.val));
            }
        }
    }
            
    
//    for (auto &a : c->args) {
    for (int i = 0; i < c->args.size(); ++i) {
        replace_code(c->args[i], d); // ?;
    }
}

// [notationのマッチ関数] 変数にマッチ
shared_ptr<Code> match_variable(shared_ptr<Code> &c, int &index, bool checkonly) {

    if (index == 0) {
        cout << "variable: " << *c << endl;
        index++;
        return c;
    } else {
        if (c->args.size() < index)
        {
            index = -1;
            return c;
        } else {
            cout << "variable: " << *c->args[index-1] << endl;
            return c->args[(index++)-1];
        }
    }
}

// [notationのマッチ関数] 自由変数にマッチ
shared_ptr<Code> match_free_variable(shared_ptr<Code> &c, const vector<string> &rest_config, int &index, bool checkonly) {
    // 最後までtmpという新しいCodeを作ってその引数に入れていく
    shared_ptr<Code> tmp;

    if (index == 0) {
        tmp = shared_ptr<Code>(new Code);
        tmp->l = c->l;
        tmp->lit = c->lit;

        // TODO: srcを変えるなら前と同じようにここで
    } else {
        tmp = make_shared<Code>(*c->args[index-1]);
    }

    index++;
    
    for (auto e = next(c->args.begin(), index-1); e != c->args.end(); e++) {
        if (check_match_notation(c, rest_config, index)) {
            return tmp;
        }
        tmp->args.push_back(*e);
        index++;
    }

    cout << "Free variable: " << *tmp << endl;
    
    return tmp;
}

// [notationのマッチ関数] 変数に貪欲にマッチ
shared_ptr<Code>
match_free_variable_greedily(shared_ptr<Code> &c, int &index, bool checkonly) {
    // 最後までtmpという新しいCodeを作ってその引数に入れていく
    shared_ptr<Code> tmp = shared_ptr<Code>(new Code);

    if (index == 0) {
        tmp->args.push_back(c);
        index ++;
    }
    
    for (auto e = next(c->args.begin(), index-1); e != c->args.end(); e++) {
        tmp->args.push_back(*e);
        index++;
    }

    cout << "Free variable: " << *tmp << endl;
    
    return tmp;
}

// [notationのマッチ関数] 指定されたトークンにマッチ
shared_ptr<Code> match_token(shared_ptr<Code> &c, string token, int &index, bool checkonly) {
    if (index == 0) {
        if (c->lit.val != token) {
            index = -1;
            return c;
        } else {
            cout << "Token: " << *c << endl;
        
            index ++;
            return c;
        }
    } else {
        if (c->args.size() < index ||
            c->args[index-1]->lit.val != token)
        {
            index = -1;
            return c;
        } else {
        
            cout << "Token: " << *c->args[index-1] << endl;
            return c->args[(index++)-1];
        }
    }
}

// コード c にnotationを適用する
bool apply_notation(shared_ptr<Code> &code, const Notation &notation) {
    // length check ?

    // map<string, shared_ptr<Code>> d;
    // int hayidx = 0;
    // for (auto c : notation.config) {
    //     if (is_notation_variable(c)) {
    //         d[c] = match_variable(code, hayidx, false);
    //     } else if (is_notation_free_variable(c)) {
    //         d[c] = match_free_variable(code, hayidx, false);
    //     } else {
    //         d[c] = match_token(code, hayidx, false);
    //     }

    //     if (hayidx == -1) return false;
    // }

    return true;
}

// gnotationがcodeにマッチするかどうか
// するなら(定義セット, true)を返す
tuple<map<string, shared_ptr<Code>>, vector<shared_ptr<Code>>, bool>
match_notation_greedily(shared_ptr<Code> &code, const vector<string> &config) {
    map<string, shared_ptr<Code>> d;
    int hayidx = 0;
    
    for (auto c = config.begin(); c != config.end(); ++c) {
        if (is_notation_variable(*c)) {
            d[*c] = match_variable(code, hayidx, false);
        } else if (is_notation_free_variable(*c)) {
            if (c == prev(config.end())) {
                d[*c] = match_free_variable_greedily(code, hayidx, false);
            } else {
                d[*c] = match_free_variable(code, vector<string>(next(c), config.end()), hayidx, false);
            }
        } else {
            d[*c] = match_token(code, *c, hayidx, false);
        }

        if (hayidx == -1) return make_tuple(d, vector<shared_ptr<Code>>(), false);
    }

    cout << "doen" << endl;
    
    return make_tuple(d, vector<shared_ptr<Code>>(next(code->args.begin(), hayidx-1), code->args.end()), true);
}

// codeにgnotationを適用する
bool apply_notation_greedily(shared_ptr<Code> &code, const Notation &notation) {
    // length check ?

    map<string, shared_ptr<Code>> defs;
    vector<shared_ptr<Code>> remains;

    {
        auto tmp = match_notation_greedily(code, notation.config);
        if (!get<2>(tmp)) return false;
        defs = get<0>(tmp);
        remains = get<1>(tmp);
    }

    code = shared_ptr<Code> (new Code);
    code->deep_copy_from(*notation.to);

    cout << "before" << endl;
    cout << *code << endl << endl;

    for (auto it = defs.begin(); it != defs.end(); it++) {
        cout << "{" << it->first << "}" << endl;
        cout << *it->second << endl << endl;
    }

    fix_defs_fv(defs);
    replace_code(code, defs);
    copy(remains.begin(), remains.end(), std::back_inserter(code->args));

    cout<< "after" << endl;
    cout << *code << endl << endl;

    return true;
}

bool
check_match_notation (shared_ptr<Code> &code, const vector<string> &config, int index) {
    // // length check ?

    int hayidx = index;
    
    for (auto c = config.begin(); c != config.end(); ++c) {
        if (is_notation_variable(*c)) {
            match_variable(code, hayidx, true);
        } else if (is_notation_free_variable(*c)) {
            if (c == prev(config.end())) {
                match_free_variable_greedily(code, hayidx, true);
            } else {
                match_free_variable(code, vector<string>(c, config.end()), hayidx, true);
            }
        } else {
            match_token(code, *c, hayidx, true);
        }

        if (hayidx == -1) return false;
    }
    
    return true;
}
