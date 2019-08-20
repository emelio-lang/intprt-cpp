/* ========================================================================
   $File: util.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"

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


ostream& operator<<(ostream& stream, const Literal& lit) {
    stream << "<" << lit.val << ">";
    return stream;
}

ostream& operator<<(ostream& stream, Lambda* l) {
    stream << "(λ ";
    for (auto a : l->argnames) stream << a << " ";
    stream << l->body << ")" << endl;
    return stream;
}

ostream& operator<<(ostream& stream, const Code& c) {
    if (c.l) {
        stream << c.l;
    } else if (c.lit.val != "") {
        stream << "(λ " << c.lit << ")";
    }
    stream << "[";
    for (auto a : c.args) stream << a << ",";
    stream << "]";
    return stream;
}

ostream& operator<<(ostream& stream, const Lambda& l) {
    stream << "(λ ";
    for (auto a : l.argnames) stream << a << " ";
    stream << l.body << ")" << endl;
    return stream;
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
