#if !defined(CODEGEN_H)
/* ========================================================================
   $File: codegen.h $
   $Date: Dec 12 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"

enum GuardType { GTYPE_COUNTABLE_FINITE, GTYPE_FINITE };
struct Guard {
    vector<pair<string, shared_ptr<Code>>> finites;
    shared_ptr<Code> countable;
};
Guard get_guard(const vector<shared_ptr<Code>> &args);
GuardType get_guard_type(const vector<shared_ptr<Code>> &args);


class set_arity {
private:
    map<string, int> bind;
    stack<shared_ptr<Code>> argstack;
public:
    set_arity(map<string, int> *pb = nullptr, stack<shared_ptr<Code>> *pa = nullptr) {
        if (pa) argstack = *pa;
        if (pb) bind = *pb;
    }
    ~set_arity() {
    }
    void operator () (const shared_ptr<Code> c);
};

class codegen {
private:
    Code callable;

    deque<shared_ptr<Code>> argstack;
    map<string, shared_ptr<Code>> bind;
    set<string> in_recursion;
    bool is_root = true;
    
public:
    codegen() {}
    ~codegen() {}
    pair<string,string> operator () (const shared_ptr<Code> c);
};

struct StackLanguage {
    vector<string> root;
    vector<string> env;
};

class codegen2 {
private:
    Code callable;

    unsigned insfunc_counter = 0;
    unsigned bsoffset = 0;

    deque<string> unbinded;
    set<string> defined;
    map<string, shared_ptr<Code>> bind;
    deque<shared_ptr<Code>> argstack;
//    map<string, shared_ptr<Code>> bind;
    set<string> in_recursion;
    bool is_root = true;
    
public:
    codegen2() {}
    ~codegen2() {}
    StackLanguage operator () (const shared_ptr<Code> c);
};

struct BindInfo {
    double zeroarity = false;
};

struct Compiled {
    string body="", env="";
};

class codegen3 {
private:

    deque<int> argstack;
    map<string, int> bind;
    map<string, BindInfo> bindinfo;
    set<string> in_recursion;
    int stack_height = 0;
    bool is_root = true;
    
public:
    bool human = false;
    codegen3() {}
    codegen3(bool h) : human(h) {}
    ~codegen3() {}
    string print_rel(int rel);
    Compiled evoke_rel(int rel);
    Compiled literal(const string lit);
    Compiled fuse(const vector<shared_ptr<Code>> fns);
    Compiled builtin(const string &name);
    Compiled argument_evoked(const vector<shared_ptr<Code>> &args);
    Compiled operator () (const shared_ptr<Code> c);
    // v.main <+ v.env
    string compress(const Compiled &&v);
    // a += b
    void paircat(Compiled &a, const Compiled &&b);
};

class ocamlgen {
private:
public:
    ocamlgen() {}
    ~ocamlgen() {}
    string operator () (const shared_ptr<Code> c);
};

// pair<string,string> codegen(CodegenFlow);
string fasm(const vector<string> &sl);
string fasm(string);
void rename_variables(const shared_ptr<Code> c);



#define CODEGEN_H
#endif
