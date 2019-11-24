
#include "emelio.h"
#include "util.h"

int main(int argc, char **argv) {
    Tokenizer tkn;
    tkn.preset("cpp");

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

        rename_variables(root);
        
        string body = fasm(codegen({root}).first);
        string env = fasm(codegen({root}).second);

        ofstream ofs1("compiled/code.inc");
        ofstream ofs2("compiled/env.inc");

        ofs1 << body << endl;
        ofs2 << env << endl;

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
//        cout << *root << endl;
        
        extract_all_notations(root, true);

        cout << *root << endl;

        rename_variables(root);

        cout << *root << endl;

//        cout << transpile(root, "c");
//        cout << codegen(root);
        cout << codegen({root});

        
//         cout << *root << endl;


        return 0;
    }
}

/*
  ((|f g| (|x| f (g x))) negate negate) 3
  
 */
