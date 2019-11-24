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

    void deep_copy_from(const Code& other);
    vector<string> plain_string();
};

struct Lambda {
    vector<string> argnames {};
    shared_ptr<Code> body;

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
pair<string,string> codegen(CodegenFlow);
string fasm(string);
void rename_variables(const shared_ptr<Code> c);


#define EMELIO_H 1
#endif
