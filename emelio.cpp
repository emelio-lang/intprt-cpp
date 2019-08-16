
#include "emelio.h"

Tokenizer tkn;

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


void test(bool verbose = false) {
    ifstream ifs("testcases.em");

    string cd, ans, ito, kara;

    if (verbose) cout << "wwwwwww TESTING wwwwwwww" << endl;


    while (true) {
        if (!getline(ifs, ito)) break;
        if (!getline(ifs, cd)) break;
        if (!getline(ifs, ans)) break;
        getline(ifs, kara);
        
        if (verbose) {
            cout << ito << endl;
            cout << cd << endl;
        } else {
            cout << "Testing " << cd << " --- ";
        }

        tkn.tokenize(cd);
        try {
            // TODO: コードの実行
//            string res = exec_code(parse_code(tkn.tokens)).cal;

            // if (res == ans) {
            //     cout << "\033[1;32mPassed.\033[0m" << endl;
            // } else {
            //     cout << "\033[1;31mError!\033[0m " << endl << "Expected '" << ans << "', but '" << res << "'." << endl;
            //     cout << ito << endl;
            // }
        } 
        catch (std::exception& e)
        {
            cerr << "\033[1;31mError!\033[0m " << endl << "Exception caught : " << e.what() << endl;
            cout << ito << endl;
        }


    }
}


int main() {
    tkn.preset("cpp");
    tkn.tokenize_file("test.em");

    showv(tkn.tokenvals);

    ProgramData pd = parse(tkn.tokenvals).first;

    // test();

    return 0;
}
