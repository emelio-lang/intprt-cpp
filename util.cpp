/* ========================================================================
   $File: util.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"

#include <cctype>
#include <locale>
#include <sstream>





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

void Code::deep_copy_from(const Code& other) {
    lit = other.lit;
    src = other.src;

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

void Lambda::deep_copy_from(const Lambda& other) {
    argnames = other.argnames;

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

bool is_computed(const shared_ptr<Code> &c) {
    return (c && !c->l);
}

// bool is_fusable(const shared_ptr<Code> &c) {
//     return c->args.size() == 0;
// }

//Code::~Code() { if (l) delete l; }


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

bool is_literal(const std::string& s) {
    return is_number(s);
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


ostream& operator<<(ostream& stream, const Literal& lit) {
    stream << "<" << lit.val << ">";
    return stream;
}

ostream& operator<<(ostream& stream, Lambda *l) {
    stream << "(λ ";
    for (auto a : l->argnames) stream << a << " ";
    stream << *l->body << ")" << endl;
    return stream;
}

ostream& operator<<(ostream& stream, const Code& c) {
    if (c.l) {
        stream << *c.l;
    } else if (c.lit.val != "") {
        stream << "(λ " << c.lit << ")";
    }
    stream << "[";
    for (const auto &a : c.args) stream << *a << ",";
    stream << "]";
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
