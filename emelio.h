#pragma once
#ifndef EMELIO_H
/* ========================================================================
   $File: emelio.h $
   $Date: Jul 07 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include <iterator>
#include <memory>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <array>
#include <map>
#include <set>
#include <cassert>
#include <variant>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"


#define CONTAINS(c,v) ((c).find(v) != (c).end())
#define REFEQUAL(a,b) (&(a) == &(b))
#define INDEXOF(c,v) (distance((c).begin(), find((c).begin(), (c).end(), v)))
#define FOR(i,a,b) for(int i=a;i<b;++i)
#define REP(i,n) FOR(i,0,n)


#define internal_global static
#define persistent static

#define ARG(x) const x&
#define MUT_ARG(x) x&

// Type = String | shared_ptr<TypeSignature>
// TypeSignature = { from: [Type], to: Type }

struct TypeSignature;
typedef variant<string, shared_ptr<TypeSignature>> Type;

struct TypeSignature {
    deque<Type> from;
    Type to;

    TypeSignature() {}
    ~TypeSignature() {}
    TypeSignature(deque<Type> a, Type b) : from(a), to(b) {} //TODO
    TypeSignature(Type a) {
        from = {};
        to = a;
    }
    TypeSignature(deque<Type> a) {
        to = a.back();
        a.pop_back();
        from = a;
    }

    int arity() const { return from.size(); }
    int functional() const { return arity() != 0; }
    int funcgen() const {
        if (holds_alternative<shared_ptr<TypeSignature>>(to)) {
            return get<shared_ptr<TypeSignature>>(to)->functional();
        }
        return false;
    }
    TypeSignature outcome() const { return TypeSignature { {}, to }; }
    void apply(int n) {
        assert(n <= int(from.size()));

        for (int i = 0; i < n; i++) from.pop_front();

        // None -> (a -> a) みたいになっている場合は a -> a としたい
        // apply はこのようになる可能性のある操作
        while (from.size() == 0) {
            if (holds_alternative<shared_ptr<TypeSignature>>(to)) {
                auto typesig = get<shared_ptr<TypeSignature>>(to);
                from = typesig->from;
                to = typesig->to;
            } else break;
        }
    }
    void wrap(const TypeSignature &typesig) {
        if (typesig.from.size() == 0) {
            from.push_front(typesig.to); // TODO
        } else {
            from.push_front(shared_ptr<TypeSignature>(new TypeSignature));
            get<shared_ptr<TypeSignature>>(from.front())->deep_copy_from(typesig);
        }
    }
    void normalize() {
        TypeSignature res = *this;
        while (holds_alternative<shared_ptr<TypeSignature>>(res.to)) {
            auto typesig = get<shared_ptr<TypeSignature>>(res.to);
            for (int i = 0; i < typesig->from.size(); i++) {
                res.from.push_back(typesig->from[i]);
            }
            res.to = typesig->to;
        }

        for (int i = 0; i < res.from.size(); i++) {
            Type res1 = from[i];
            if (holds_alternative<shared_ptr<TypeSignature>>(res1)) {
                shared_ptr<TypeSignature> e = get<shared_ptr<TypeSignature>>(res1);
                if (holds_alternative<string>(e->to)) {
                    res1 = e->to;
                }
            }
            from[i] = res1;
        }
    }
    // NOTE: deep copyはしないのでこれを変更しないように
    const TypeSignature normalized() const {
        TypeSignature res = *this;
        while (holds_alternative<shared_ptr<TypeSignature>>(res.to)) {
            auto typesig = get<shared_ptr<TypeSignature>>(res.to);
            for (int i = 0; i < typesig->from.size(); i++) {
                res.from.push_back(typesig->from[i]);
            }
            res.to = typesig->to;
        }
        return res;
    }
    TypeSignature arg(int n) const {
        assert(n <= int(from.size()-1));
        return TypeSignature { {} , {from[n]} };
    }
    string to_string() const {
        string res = "";
        for (auto f : from) {
            if (holds_alternative<string>(f)) { res += get<string>(f) + "-"; }
            else { res += "(" + get<shared_ptr<TypeSignature>>(f)->to_string() + ")-"; }
        }
        if (from.size() != 0) res += ">";
        if (holds_alternative<string>(to)) { res += get<string>(to); }
        else {
            res += "("+get<shared_ptr<TypeSignature>>(to)->to_string()+")";
        }
        return res;
    }
    void deep_copy_from(const TypeSignature &other) {
        if (holds_alternative<string>(other.to)) {
            to = other.to;
        }
        else {
            to = shared_ptr<TypeSignature>(new TypeSignature);
            get<shared_ptr<TypeSignature>>(to)->deep_copy_from(*get<shared_ptr<TypeSignature>>(other.to));
        }

        from = {};
        for (auto f : other.from) {
            if (holds_alternative<string>(f)) {
                from.push_back(f);
            } else {
                from.push_back(shared_ptr<TypeSignature>(new TypeSignature));
                get<shared_ptr<TypeSignature>>(from.back())->deep_copy_from(*get<shared_ptr<TypeSignature>>(f));
            }
        }
    }
};
inline bool operator==(const TypeSignature &lhs, const TypeSignature &rhs) {
    return (lhs.from == rhs.from) && (lhs.to == rhs.to);
}
inline bool operator!=(const TypeSignature &lhs, const TypeSignature &rhs) {
    return ! (lhs == rhs);
}


struct ParserFlow {
    vector<string> &tknvals;
    int idx;
};
struct Code;
struct Literal;
struct Lambda;

struct Literal {
    string val;
};

struct TknvalsRegion {
    vector<string>::iterator beg, end;

    size_t size() const {
        return distance(this->beg, this->end);
    }
};

struct Code {
    shared_ptr<Lambda> l;
    Literal lit;
    vector<shared_ptr<Code>> args;

    // NOTE: tknvalsは変更されないことを想定しています
    TknvalsRegion src;

    TypeSignature type;


    int arity = 0;

    void deep_copy_from(const Code& other);
    vector<string> plain_string();
};

struct ArgQuality {
    bool recursive = false;
};


struct Lambda {
    vector<string> argnames {};
    shared_ptr<Code> body;
    vector<int> argarities {};
    vector<ArgQuality> argqualities {};
    vector<TypeSignature> argtypes {};

    TypeSignature type;


//    vector<shared_ptr<Lambda>> fused;

    void deep_copy_from(const Lambda& other);
};

struct CodegenFlow {
    shared_ptr<Code> c;
    deque<string> unbinded;
    deque<shared_ptr<Code>> argstack;
    set<string> defined;
    unsigned fstack_offset = 0;  // for recursion
    set<string> in_recursion;
    map<string, shared_ptr<Code>> bind;
};






unique_ptr<Code> code(ParserFlow& p);
//pair<ProgramData,int> parse(ARG(vector<string>) tknvals, int initial_idx = 0, string basename = "");
void reduction(shared_ptr<Code> code, bool silent = false);
void extract_all_notations(shared_ptr<Code> c, bool silent = false);

#define EMELIO_H 1
#endif
