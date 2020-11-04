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

using namespace std;

void ASSERT(bool glass, std::string msg);

vector<string> split(const string &s, char delim);
std::string join(const std::vector<std::string>& v, const char* delim = 0);
    
template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(const std::basic_string<Char, Traits, Allocator> s, size_t n);

template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(size_t n, const std::basic_string<Char, Traits, Allocator>& s);


template<typename Key, typename T>
std::ostream& operator<<(std::ostream& stream, const std::map<Key,T>& m);

bool is_number(const std::string& s);
bool is_string_literal(const std::string& s);
bool is_literal(const std::string& s);
bool is_builtin(const std::string& s);
char asciitolower(char in);
std::string tolower(std::string);

std::string random_string( size_t length );
std::string random_sane_string( size_t length );
std::string random_saneupper_string( size_t length );

bool is_all_upper(string &s);

int maxzero(int n);

#define UTIL_H
#endif
