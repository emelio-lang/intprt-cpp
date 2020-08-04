/* ========================================================================
   $File: emelio-util.cpp $
   $Date: Jul 07 2020 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2020 by Creative GP. All Rights Reserved. $
   ======================================================================== */

/*
  emelio.hで定義されている、emelioで使用するデータ構造に対するユーティリィー関数をここに書いていく
 */

#include "emelio.h"
#include "util.h"

#include <cctype>
#include <locale>
#include <sstream>

bool is_computed(const shared_ptr<Code> &c) {
    return (c && !c->l);
}

// Code::Code(const Code& other) {
//     lit = other.lit;
//     src = other.src;
    
//     if (other.l) {
//         l = unique_ptr<Lambda>(new Lambda);
//         *l = *other.l;
//     } else {
//         l = unique_ptr<Lambda>(nullptr);
//     }

//     for (Code c : other.args) {
//         args.push_back(Code(c));
//     }
// }


string get_eventual_fnname(shared_ptr<Code> c) {
    if (!c->l) { return c->lit.val; }
    else return get_eventual_fnname(c->l->body);
}

vector<string> Code::plain_string() {
    vector<string> res = {};

    if (this->lit.val != "") res.push_back(this->lit.val);

    for (auto a : this->args) {
        vector<string> const tmp = a->plain_string();
        res.reserve(res.size() + distance(tmp.begin(),tmp.end()));
        res.insert(res.end(),tmp.begin(),tmp.end());
    }

    if (this->l) {
        vector<string> const tmp = this->l->body->plain_string();
        res.reserve(res.size() + distance(tmp.begin(),tmp.end()));
        res.insert(res.end(),tmp.begin(),tmp.end());
    }

    return res;
}


void Code::deep_copy_from(const Code& other) {
    lit = other.lit;
    src = other.src;
    arity = other.arity;
    ::deep_copy_from(type, other.type);

    // TODO: srcもコピーしないと

    // もうすでにnullでないptrを持っているならそれを活かす
    if (other.l) {
        if (!l) l = shared_ptr<Lambda>(new Lambda);
        l->deep_copy_from(*other.l);
    } else {
        l = shared_ptr<Lambda>(nullptr);
    }

    args.clear();
    for (const shared_ptr<Code> &c : other.args) {
        shared_ptr<Code> copied (new Code);
        copied->deep_copy_from(*c);
        args.push_back(copied);
    }

}

bool Code::is_type() const {
    return !MATCHS(TypeNull)(rawtype);
}

void Lambda::deep_copy_from(const Lambda& other) {
    argnames = other.argnames;
    argarities = other.argarities;
    argqualities = other.argqualities;
    ::deep_copy_from(type, other.type);

    if (other.body) {
        if (!body) body = shared_ptr<Code>(new Code);
        body->deep_copy_from(*other.body);
    }

    // fused.clear();
    // for (const shared_ptr<Lambda> &l : other.fused) {
    //     shared_ptr<Lambda> copied (new Lambda);
    //     copied->deep_copy_from(*l);
    //     fused.push_back(copied);
    // }
}


ostream& operator<<(ostream& stream, const Literal& lit) {
    stream << "<" << lit.val << ">";
    return stream;
}

ostream& operator<<(ostream& stream, Lambda *l) {
    stream << "(λ:" << to_string(l->type) << " ";
    for (auto a : l->argnames) stream << a << " ";
    stream << *l->body << ")" << endl;
    return stream;
}

ostream& operator<<(ostream& stream, const Code& c) {
//    stream << c.arity;
    
    if (c.l) {
        stream << *c.l;
    } else if (c.lit.val != "") {
        stream << "(λ " << c.lit << ")";
    } else if (c.is_type()) {
        stream << "#" << to_string(c.rawtype) << endl;
    }
    
    stream << "[";
    for (const auto &a : c.args) stream << *a << ",";
    stream << "]";

    stream << ":" << to_string(c.type);
    return stream;
}

ostream& operator<<(ostream& stream, const pair<string,string>&& p) {
    stream << "(" << p.first << ", " << p.second << ")" << endl;
    return stream;
}

ostream& operator<<(ostream& stream, const Lambda& l) {
    // if (l.fused.size() == 0) {
        stream << "(λ ";
        for (auto a : l.argnames) stream << a << " ";
        if (l.body) stream << *l.body << ")" << endl;
        else stream << "--NO BODY LAMBDA--" << ")" << endl;
    // } else {
    //     stream << "(λ-fuse ";
    //     for (const shared_ptr<Lambda> fus : l.fused) {
    //         stream << *fus;
    //     }
    //     stream << ")" << endl;
    // }
    return stream;
}
