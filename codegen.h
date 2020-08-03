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

class set_type {
private:
    map<string, TypeSignature> bind;
    map<string, shared_ptr<Lambda>> type_constructors;
    stack<shared_ptr<Code>> argstack;
public:
    set_type(map<string, TypeSignature> *pb = nullptr, stack<shared_ptr<Code>> *pa = nullptr) {
        if (pa) argstack = *pa;
        if (pb) bind = *pb;
    }
    ~set_type() {
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
    shared_ptr<Code> code;
};

struct Compiled {
    string body="", env="";
};

class codegen3 {
private:

    // バインド保留のコード と それの実体（または実体を指すポインタ）があるスタックの絶対位置 のペア
    deque<int> argstack;
    deque<shared_ptr<Code>> argstack_code;

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
    Compiled copy_rel(int rel);
    Compiled literal(const string lit);
    Compiled fuse(const vector<shared_ptr<Code>> fns);
    Compiled builtin(const string &name);
    Compiled all_arguments_evoked(const vector<shared_ptr<Code>> &args);
    Compiled argument_evoked(const shared_ptr<Code> &args);
    Compiled operator () (const shared_ptr<Code> c, const shared_ptr<Lambda> enviroment);
    // v.main <+ v.env
    string compress(const Compiled &&v);
    // a += b
    void paircat(Compiled &a, const Compiled &&b);
};

class codegen4 {
private:

    // バインド保留のコード と それの実体（または実体を指すポインタ）があるスタックの絶対位置 のペア
    deque<pair<string,shared_ptr<Code>>> argstack;
    deque<shared_ptr<Code>> argstack_code;

    map<string, int> bind;
    map<string, BindInfo> bindinfo;
    set<string> in_recursion;
    int stack_height = 0;
    bool is_root = true;

public:
    bool human = false;
    codegen4() {}
    codegen4(bool h) : human(h) {}
    ~codegen4() {}
    string print_rel(int rel);
    Compiled evoke_rel(int rel);
    Compiled copy_rel(int rel);
    Compiled literal(const string lit);
    Compiled fuse(const vector<shared_ptr<Code>> fns);
    Compiled builtin(const string &name);
    Compiled all_arguments_evoked(const vector<shared_ptr<Code>> &args);
    Compiled argument_compiled(const string &ident , const shared_ptr<Code> &arg);
    Compiled operator () (const shared_ptr<Code> c, const shared_ptr<Lambda> enviroment);
    // v.main <+ v.env
    string compress(const Compiled &&v);
    // a += b
    void paircat(Compiled &a, const Compiled &&b);
};

class codegen5 {
private:

    // バインド保留のコード と それの実体（または実体を指すポインタ）があるスタックの絶対位置 のペア
    deque<pair<string,shared_ptr<Code>>> argstack;
    deque<shared_ptr<Code>> argstack_code;

    deque<string> reserved;
    map<string, string> reserved_bind;

    map<string, int> bind;
    map<string, BindInfo> bindinfo;
    set<string> in_recursion;
    int stack_height = 0;
    bool is_root = true;

public:
    bool human = false;
    codegen5() {}
    codegen5(bool h) : human(h) {}
    ~codegen5() {}
    string print_rel(int rel);
    Compiled evoke_rel(int rel);
    Compiled copy_rel(int rel);
    Compiled literal(const string lit);
    Compiled fuse(const vector<shared_ptr<Code>> fns);
    Compiled builtin(const string &name);
    Compiled all_arguments_evoked();
    string get_name(string name);
    Compiled argument_evoked( const shared_ptr<Code> &arg);
    Compiled argument_compiled(const string &ident , const shared_ptr<Code> &arg);
//    string argument_signature(const Type &);
//    string type_signature(const deque<AtomType> &type, const string &name);

    string vardef(const string &name, const TypeSignature &typesig);
    string argdef(const TypeSignature &typesig, bool reserve = false);

    Compiled operator () (const shared_ptr<Code> c, const shared_ptr<Lambda> enviroment, bool);
    // v.main <+ v.env
    string compress(const Compiled &&v);
    // a += b
    void paircat(Compiled &a, const Compiled &&b);
};

class codegen6 {
private:
    stack<string> barstack;
    stack<shared_ptr<Code>> argstack;
    static map<string, shared_ptr<Code>> bind;
    static map<string, shared_ptr<Lambda>> type_constructors; // TODO: スコープいいの？
    // static map<string, DataStructure> data_bind;
    int pseudo_func_counter = 0;

public:
    codegen6() {}
    ~codegen6() {}
    // string print_data_structure(const DataStructure &ds);
    // DataStructure parse_data_structure(const shared_ptr<Code> &c);
    string print_type_to(const TypeSignature &ty);
    string print_type_from(const deque<TypeSignature> &tys, const shared_ptr<Lambda> &lam);
    string print_decl(string name, const TypeSignature &type, bool pointer=false);
    string print_def(string name, const shared_ptr<Code>& code);
    Compiled operator () (const shared_ptr<Code> &);
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
