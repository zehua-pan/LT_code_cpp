#include <iostream>
#include <vector>

using namespace std;

struct symbol
{
    // constructor
    symbol() = default;
    symbol(int i, int d, long f): symbolID(i), degree(d), filesize(f){}
    // copy control
    symbol(const symbol& s) : symbolID(s.symbolID), degree(s.degree), filesize(s.filesize){}
    symbol& operator=(const symbol& s)
    {
        symbolID = s.symbolID;
        degree = s.degree;
        filesize = s.filesize;
        cout << "copy assignment" << endl;
        return *this;
    }
    int symbolID;
    int degree;
    long filesize;
};

int main()
{
    // symbol* sym = new symbol(1, 1, 1);
    // vector<symbol> symbols;
    // symbols.push_back(std::move(*sym));


    const string path("a");
    const string name("b");
    string c = path + name;
    cout << c << endl;
    return 0;
}

