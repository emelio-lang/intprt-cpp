#if !defined(NOTATION_H)
/* ========================================================================
   $File: notation.h $
   $Date: Sep 09 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

struct Notation {
    vector<string> config;
    shared_ptr<Code> to;
};

inline bool is_notation_variable(const string& s);
inline bool is_notation_free_variable(const string& s);

void pad_notation_config(vector<string>& config);
void replace_code(shared_ptr<Code> c, const map<string, shared_ptr<Code>> &d);

bool apply_notation(shared_ptr<Code> &code, const Notation& notation);
bool apply_notation_greedily(shared_ptr<Code> &code, const Notation& notation);

shared_ptr<Code> match_variable(shared_ptr<Code> &c, int &index, bool checkonly = false);
shared_ptr<Code> match_free_variable(shared_ptr<Code> &c, const vector<string> &rest_config, int &index, bool checkonly = false);
shared_ptr<Code> match_free_variable_greedily(shared_ptr<Code> &c, int &index, bool checkonly = false);
shared_ptr<Code> match_token(shared_ptr<Code> &c, string token, int &index, bool checkonly = false);
tuple<map<string, shared_ptr<Code>>, vector<shared_ptr<Code>>, bool> match_notation_greedily(shared_ptr<Code> &code, const vector<string> &config);
bool check_match_notation(shared_ptr<Code> &code, const vector<string> &config, int index = 0);

#define NOTATION_H
#endif
