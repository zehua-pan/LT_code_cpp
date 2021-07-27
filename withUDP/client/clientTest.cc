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
    // this test case have to be used with testReceiver() 
    // in "../server/serverTest.cc"
    Sender sender;
    int limit = 100;
    int interval = 10;
    int sentSymN = 0;
    string filename = "a.txt";
    struct symbol newsym(1, 1, 1);
    strcpy(newsym.filenameArray, filename.c_str());
    while(1)
    {
        sender.send(newsym);
        ++sentSymN;
        cout << "have sent : " << sentSymN << " symbols" << endl;
        if(sentSymN >= limit && sentSymN % interval == 0)
        {
            cout << "interval touch" << endl;
            if(sender.checkEndMsg())
            {
                cout << "receive end message from server, end" << endl;
                break;
            }
        }
    }
}

void testEncoder()
{
    // this test case have to be used with testDecoder() 
    // in "../server/serverTest.cc"
    string dataSrcPath("../../data/sendData/");
    string filename("file14.mp4"); 
    Encoder encoder(dataSrcPath, filename);
    encoder.read_file();
    encoder.encode();
}

int main()
{
    testEncoder();
    // testSender();
    return 0;
}
