#if !defined(UTIL_H)
/* ========================================================================
   $File: util.h $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */


#include <iterator>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <array>
#include <map>

vector<string> split(const string &s, char delim);
    
template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(const std::basic_string<Char, Traits, Allocator> s, size_t n);

template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(size_t n, const std::basic_string<Char, Traits, Allocator>& s);

bool is_number(const std::string& s);
bool is_literal(const std::string& s);

bool is_computed(const shared_ptr<Code> &c);


std::string random_string( size_t length );
std::string random_sane_string( size_t length );
std::string random_saneupper_string( size_t length );
bool is_all_upper(string &s);

int maxzero(int n);

ostream& operator<<(ostream& stream, const Literal&);
ostream& operator<<(ostream& stream, const Code&);
ostream& operator<<(ostream& stream, const Lambda&);
ostream& operator<<(ostream& stream, Lambda*);

#define UTIL_H
#endif
