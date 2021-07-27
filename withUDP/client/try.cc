#include <iostream>
#include <cstring>
#include <vector>

using namespace std;

const int FILENAME_MAXLEN = 32;

struct symbol
{
    // constructor
    symbol() = default;
    symbol(int i, int d, long f): symbolID(i), degree(d), filesize(f){}
    // copy control
    symbol(const symbol& s) : symbolID(s.symbolID), degree(s.degree), filesize(s.filesize){copyNewData(s);}
    symbol& operator=(const symbol& s);
    // member functions
    void copyNewData(const symbol& s);
    // data member
    int symbolID;
    int degree;
    long filesize;
    char filenameArray[FILENAME_MAXLEN];
};


int main()
{
    string filename("a.txt");
    struct symbol newsym(1, 1, 1);
    cout << sizeof(newsym) << endl;
    strcpy(newsym.filenameArray, filename.c_str());
    string newname(newsym.filenameArray);
    cout << newname << endl;
    return 0;
}

