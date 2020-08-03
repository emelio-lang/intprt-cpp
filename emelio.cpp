
#include "emelio.h"
#include "util.h"
#include "codegen.h"

int main(int argc, char **argv) {
    Tokenizer tkn;
    tkn.set("specials", "!#$%&()-^\\@[;:],./=~|{+*}<>?");
    tkn.set("escaper", "\"'`");
    tkn.set("ignores", "");
    tkn.set("ignoresplit", " \t\n");

    if (argc == 2) {
        tkn.tokenize(argv[1]);
        ParserFlow pf = {tkn.tokenvals, 0};
        shared_ptr<Code> root = code(pf);
        reduction(root, true);

        cout << root->lit.val << endl;

        return stoi(root->lit.val);
    } else if (argc == 3) {
        auto back = cout.rdbuf();
        cout.rdbuf(NULL);

        tkn.tokenize_file(argv[2]);
        ParserFlow pf = {tkn.tokenvals, 0};
        shared_ptr<Code> root = code(pf);
        extract_all_notations(root, true);

        // rename_variables(root);
        // auto tmp = codegen2()(root);
        // auto body = fasm(tmp.root);
        // auto env = fasm(tmp.env);

        // ofstream ofs1("compiled/code.inc");
        // ofstream ofs2("compiled/env.inc");

        // ofs1 << body << endl;
        // ofs2 << env << endl;
        
        cout.rdbuf(back);
        
//        cout << e << endl;
    } else if (argc > 2) {
        std::cin >> std::noskipws;
        std::istream_iterator<char> it(std::cin);
        std::istream_iterator<char> end;
        string input(it, end);

        tkn.tokenize(input);
        ParserFlow pf = {tkn.tokenvals, 0};
        shared_ptr<Code> root = code(pf);
        reduction(root, true);

        cout << root->lit.val << endl;

        return stoi(root->lit.val);
    } else {
    
        tkn.tokenize_file("test.em");

//        showv(tkn.tokenvals);

        ParserFlow pf = {tkn.tokenvals, 0};

        shared_ptr<Code> root = code(pf);
        cout << *root << endl;
        // set_type()(root);
        // cout << *root << endl;
        
        // extract_all_notations(root, true);
        // cout << *root << endl;
        // rename_variables(root);
//        set_arity()(root);
//        set_type()(root);

//        cout << *root << endl;

//        cout << transpile(root, "c");
//        cout << codegen(root);
//        cout << "prepare name hash:\n";
//        Compiled result;
//        result = codegen6()(root);

//        result = codegen5()(root, nullptr, true);


        // ofstream ofs1("compiled/code.c");
        // ofstream ofs2("compiled/env.c");
        // ofs1 << result.body << endl;
        // ofs2 << result.env << endl;

        // ofstream ofs3("compiled/code5.c");
        // ofs3 << "#include <stdio.h>" << endl;
        // ofs3 << "int __main__() { " << endl;
        // ofs3 << result.env << endl << endl;
        // ofs3 << "return " << result.body << ";" << endl;
        // ofs3 << "}" << endl;
        // ofs3 << "void main() { printf(\"%d\\n\", __main__()); }" << endl << endl;

        // cout << "#include <stdio.h>" << endl;
        // cout << "int __main__() { " << endl;
        // cout << result.env << endl;
        // cout << "return " << result.body << ";" << endl;
        // cout << "}" << endl;
        // cout << "void main() { printf(\"%d\\n\", __main__()); }" << endl << endl;


//         cout << *root << endl;


        return 0;
    }
}

/*
  ((|f g| (|x| f (g x))) negate negate) 3
  
 */
