#include <math.h>
#include <chrono>
#include <iostream>
#include "sender.h"
#include "receiver.h"

using namespace std;

struct test
{
    void testRobustDistribution()
    {
        // the result should be similar to the distribution of histogram in the blog
        vector<double> test = robust_distribution(25);
        for(double num : test)
        {
            for(int i = 0; i < (int)(num * 200); ++i)
                cout << "*";
            cout << endl;
        }
        cout << endl;
    }

    void testReadFile()
    {
        sender obj2("data/sendData/file4.txt");
        obj2.read_file();
        // print out some text, if input is txt file, output should be text
        for(int i = 0; i < 100; ++i)
            cout << obj2.blocks[0][i];
        cout << endl;
    }

    void testGetDegrees()
    {
        sender obj1("data/sendData/file1.txt");
        vector<int> rd = obj1.get_degrees(200);
        vector<int> tmp(obj1.DISTRIBUTION_RANGE + 1, 0);
        // result of distribution should be similar to test of robust_distribution
        for(int n : rd) ++tmp[n];
        for(int n : tmp) cout << string(n, '*') << endl;
    }

    void testGetIndices()
    {
        // should output some uniformly distributed indices, should be in the range of blocks_n and degree quantity
        sender obj1("data/sendData/file4.txt");
        obj1.read_file();
        int degree  = 4;
        int range_max = obj1.blocks_n > degree ? degree : obj1.blocks_n;
        cout << "max number of block index : " << range_max << endl;
        vector<int> indices = obj1.get_indices(4, degree);
        for(int index : indices)
            cout << index << " ";
        cout << endl;
    }

    void testEncodeWithTime()
    {
        sender obj1("data/sendData/file5.txt");
        obj1.read_file();
        double multiple = 1000.0;
        vector<symbol> symbols = obj1.encode(multiple);
    }

    void testEncode()
    {
        // the output should be text if degree is 1, if not, should be garbage;
        sender obj1("data/sendData/file4.txt");
        obj1.read_file();
        double multiple = 2.0;
        vector<symbol> symbols = obj1.encode(multiple);
        for(int i = 0; i < (int)(multiple * obj1.blocks_n); ++i)
        {
            cout << "symbol: " << i << endl;
            cout << "symbol index: " << symbols[i].symbolID;
            cout << " degree: " << symbols[i].degree << endl;;
            cout << "some data : " << endl;
            for(int j = 0; j < 20; ++j)
                cout << symbols[i].data[j];
            cout << endl;
            cout << endl;
        }
    }

    void testWhole()
    {
        string filename("file17.mp4");
        string inputPath = "../data/sendData/" + filename;
        string outputPath = "../data/receiveData/" + filename;
        
        sender sendObj(inputPath.c_str());
        sendObj.read_file();
        vector<symbol> symbols = sendObj.encode(1.7);

        receiver receiveObj(symbols);
        receiveObj.decode();
        receiveObj.writeFile(outputPath.c_str());
    }

};


int main()
{
    test t1;
    /* test encode */
    // t1.testRobustDistribution();
    // t1.testReadFile();
    // t1.testEncode();
    // t1.testEncodeWithTime();

    /* test decode */
    t1.testWhole();
    return 0;
}

