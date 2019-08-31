
#include "emelio.h"
#include "util.h"

int main(int argc, char **argv) {
    Tokenizer tkn;
    tkn.preset("cpp");

    if (argc == 2) {
        tkn.tokenize(argv[1]);
        ParserFlow pf = {tkn.tokenvals, 0};
        Code root = code(pf);
        Code redu = reduction(root, true);

        cout << redu.lit.val << endl;

        return stoi(redu.lit.val);
    } else {
    
        tkn.tokenize_file("test.em");

        showv(tkn.tokenvals);

        ParserFlow pf = {tkn.tokenvals, 0};

        Code root = code(pf);
        cout << root << endl;

        cout << reduction(root) << endl;

        return 0;
    }
}

/*
  ((|f g| (|x| f (g x))) negate negate) 3
  
 */
