/* ========================================================================
   $File: util.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */
/*
  util.hで定義されている、標準ライブラリデータ構造に対するユーティリィー関数をここに書いていく
 */

#include "util.h"
#include <cassert>


// bool is_fusable(const shared_ptr<Code> &c) {
//     return c->args.size() == 0;
// }

//Code::~Code() { if (l) delete l; }

void ASSERT(bool glass, std::string msg) {
    if (!glass) {
        cout << "Error: " + msg << endl;
        assert(glass);
    }
}


bool is_number(const std::string& s)
{
    int a;
    try {
        int a = stoi(s);
    } catch (invalid_argument& e) {
        return false;
    }
    return true;
}

bool is_string_literal(const std::string& s) {
    return s.front() == '"' && s.back() == '"';
}

bool is_literal(const std::string& s) {
    return is_number(s) || is_string_literal(s);
}

bool is_builtin(const std::string& s) {
    return s == "add" || s == "negate" || s == "concat";
}


vector<string> split(const string &s, char delim) {
    vector<string> elems;
    string item;
    for (char ch: s) {
        if (ch == delim) {
            if (!item.empty())
                elems.push_back(item);
            item.clear();
        }
        else {
            item += ch;
        }
    }
    if (!item.empty())
        elems.push_back(item);
    return elems;
}

template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(const std::basic_string<Char, Traits, Allocator> s, size_t n)
{
   std::basic_string<Char, Traits, Allocator> tmp = s;
   for (size_t i = 0; i < n; ++i)
   {
      tmp += s;
   }
   return tmp;
}

template<typename Char, typename Traits, typename Allocator>
std::basic_string<Char, Traits, Allocator> operator *
(size_t n, const std::basic_string<Char, Traits, Allocator>& s)
{
   return s * n;
}


std::string random_string( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

std::string random_sane_string( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

std::string random_saneupper_string( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

bool is_all_upper(string &s) {
    return accumulate(s.begin(), s.end(), true, [](bool acc, char i) {
                                                    return acc && isupper(i);
                                                });
}

int maxzero(int n) {
    return std::max(0, n);
}
