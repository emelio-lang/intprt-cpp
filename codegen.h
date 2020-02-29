#if !defined(CODEGEN_H)
/* ========================================================================
   $File: codegen.h $
   $Date: Dec 12 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"


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

class codegen3 {
private:

    deque<int> argstack;
    map<string, int> bind;
    set<string> in_recursion;
    int stack_height = 0;
    bool is_root = true;
    
public:
    bool human = false;
    codegen3() {}
    codegen3(bool h) : human(h) {}
    ~codegen3() {}
    string evoke_rel(int rel);
    string print_rel(int rel);
    pair<string,string> operator () (const shared_ptr<Code> c);
    pair<string,string> fnrun1(const shared_ptr<Code> c);
    pair<string,string> fnrun2(const shared_ptr<Code> c);
    pair<string,string> function_call(const shared_ptr<Code> c);
    pair<string,string> human_function_call(const shared_ptr<Code> c);
    string compress(const pair<string,string> &&v);
    void paircat(pair<string,string> &x, const pair<string,string> &&v);
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
