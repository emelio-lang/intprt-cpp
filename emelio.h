#ifndef EMELIO_H
#pragma once
/* ========================================================================
   $File: emelio.h $
   $Date: Jul 07 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include <iterator>
#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <map>

#include "Tokenizer/util.h"
#include "Tokenizer/tokenizer.h"


#define CONTAINS(c,v) ((c).find(v) != (c).end())
#define REFEQUAL(a,b) (&(a) == &(b))

template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(const std::basic_string<Char, Traits, Allocator> s, size_t n);

template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(size_t n, const std::basic_string<Char, Traits, Allocator>& s);

bool is_number(const std::string& s);
bool is_literal(const std::string& s);

#define ARG(x) const x&

struct L;

typedef string Lp;

struct L {
    Lp body;
    map<string,Lp> argbind = {};
};

struct ProgramData {
    L root;
    map<string, L> bind;
};


pair<ProgramData,int> parse(ARG(vector<string>) tknvals, int initial_idx = 0, string basename = "");


#define EMELIO_H 1
#endif
