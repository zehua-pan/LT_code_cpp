#include <iostream>
#include "globalParameters.h"
#include "DegreeGenerator.h"
#include "Sender.h"
#include "Encoder.h"

using namespace std;

void testDegreeGenerator()
{
    // generated degree should have a robust distribution of LT theory
    DegreeGenerator dg(25);
    vector<int> degrees(26, 0);
    for(int i = 0; i < 200; ++i)
        ++degrees[dg.getDegree()];
    for(int degree : degrees)
    {
        string tmp(degree, '*');
        cout << tmp << endl;
    }
}

void testSender()
{
    // should combine with testReceiver
}

void testEncoder()
{
    // suggest to start decoder in server first
    string dataSrcPath("../../data/sendData/")
    string filename("file5.txt"); 
    Encoder encoder(inPath, filename);
    encoder.read_file();
    encoder.encode();
}

int main()
{
    testDegreeGenerator();
    return 0;
}
