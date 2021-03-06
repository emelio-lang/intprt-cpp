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
#include <sstream>
#include <string>
#include <stack>
#include <array>
#include <map>
#include <set>
#include <unordered_set>
#include <cassert>
#include <variant>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"


#define CONTAINS(c,v) ((c).find(v) != (c).end())
#define REFEQUAL(a,b) (&(a) == &(b))
#define INDEXOF(c,v) (distance((c).begin(), find((c).begin(), (c).end(), v)))
#define FOR(i,a,b) for(int i=a;i<b;++i)
#define REP(i,n) FOR(i,0,n)
#define MATCH(t) holds_alternative<t>
#define MATCHS(t) holds_alternative<shared_ptr<t>>
#define PURE(t) get<t>
#define PURES(t) get<shared_ptr<t>>
#define ELIMINATE(v,x,y) ((v) == (x) ? (y) : (v))


#define internal_global static
#define persistent static

#define ARG(x) const x&
#define MUT_ARG(x) x&


struct ParserFlow {
    vector<string> &tknvals;
    int idx;
    map<string, deque<string>> polymos;
};
struct Code;
struct Literal;
struct Lambda;



// Type = String | shared_ptr<TypeSignature>
// TypeSignature = { from: [Type], to: Type }

// struct TypeSignature;
// // typedef deque<Type> AndType;
// // typedef deque<Type> OrType;
// typedef variant<string, shared_ptr<TypeSignature>/*, AndType, OrType*/> Type;
struct SpecialValue; struct Parametered; struct TypeProduct; struct TypeSum; struct TypeFn; struct TypeNull { int dummy; /*なんかvariantは空の型無理な奴らしいから*/ };
struct SpecialValue {
    string val;
};
typedef variant<shared_ptr<TypeNull>,string,SpecialValue,shared_ptr<Parametered>,shared_ptr<TypeSum>,shared_ptr<TypeProduct>,shared_ptr<TypeFn>> TypeSignature;
struct TypeSum {
    deque<TypeSignature> sums;
    // TODO: いまはこれだけ特別扱いする感じで実装してるけど、TypeSignatureにあたらしくTypeSpecialみたいなの導入して、sumsにまとめてしまったほうが見通しが良いかも（normalizeの処理とか）
    void add_type(TypeSignature ts) {
        if (find(sums.begin(), sums.end(), ts) == sums.end()) {
            sums.emplace_back(ts);
        }
    }
};
struct TypeProduct {
    deque<TypeSignature> products;
    deque<string> names;
};
struct TypeFn {
    deque<TypeSignature> from;
    TypeSignature to;
};
struct Parametered {
    string type;
    deque<TypeSignature> params;
};
bool equal(const TypeSignature &ts1, const TypeSignature &ts2, const map<string,TypeSignature> &bind = {}, int lvl = 0);
bool verify(const TypeSignature &ts1, const TypeSignature &ts2, const map<string,TypeSignature> &bind = {}, int lvl = 0);
//bool verify(const TypeSignature &ts1, const TypeSignature &ts2, const map<string,TypeSignature> &bind = {});
bool is_functional(const TypeSignature &ts1);
bool arity(const TypeSignature &ts);
bool operator==(const TypeSignature &ts1, const TypeSignature &ts2);
void deep_copy_from(TypeSignature &ts_dst, const TypeSignature &ts_src);
string to_string(const TypeSignature &typesig);
TypeSignature sumfactor(const shared_ptr<TypeSum> &sumtype);
void _normalize(TypeSignature &typesig);
void normalize(TypeSignature &typesig);
TypeSignature normalized(const TypeSignature &typesig);
TypeSignature arg(const TypeSignature &typesig, int n);
void apply(TypeSignature &typesig, const int n);
void wrap(TypeSignature &typesig, const TypeSignature &wrp);

//typedef deque<deque<variant<string, shared_ptr<DataStructure>>> DataStructure;

// struct TypeSignature {
//     shared_ptr<Code> type_code;
    
//     TypeSignature() {}
//     ~TypeSignature() {}
//     TypeSignature(shared_ptr<Code> c) : type_code(c) {}
//     TypeSignature outcome() const;
//     int arity() const;
//     bool functional() const;    
//     bool funcgen() const;
//     void apply(int n);
//     void wrap(const TypeSignature &typesig);
//     void normalize();
//     void normalized(shared_ptr<Code> c) const;
//     string to_string(const shared_ptr<Code> c) const;
//     string to_string() const;
//     void deep_copy_from(const TypeSignature& src);
// };


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
    deque<shared_ptr<Code>> args;

    // NOTE: tknvalsは変更されないことを想定しています
    TknvalsRegion src;

    TypeSignature rawtype; // Codeとして型注釈が書いてある場合 :Type
    TypeSignature type;


    int arity = 0;

    shared_ptr<Code> cRawtype;
    
    void deep_copy_from(const Code& other);
    vector<string> plain_string();
    bool is_type() const;
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
    vector<shared_ptr<Code>> cArgtypes {};

    TypeSignature type;

    vector<string> freevars {};

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
TypeSignature type_signature(ParserFlow& p);
void reduction(shared_ptr<Code> code, bool silent = false);
void extract_all_notations(shared_ptr<Code> &c, bool silent = false);
void reparse_types(shared_ptr<Code> &code, Tokenizer &tkn);

bool is_computed(const shared_ptr<Code> &c);
std::string get_eventual_fnname( shared_ptr<Code>  );

ostream& operator<<(ostream& stream, const Literal&);
ostream& operator<<(ostream& stream, const Code&);
ostream& operator<<(ostream& stream, const Lambda&);
ostream& operator<<(ostream& stream, Lambda*);

ostream& operator<<(ostream& stream, const pair<string,string>&&);


#define EMELIO_H 1
#endif
