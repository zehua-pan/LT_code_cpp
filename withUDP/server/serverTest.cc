#include <iostream>
#include <vector>
#include "./Decoder.h"
#include "./Receiver.h"
#include "globalParameters.h"

using namespace std;

void testReceiver()
{
    // this test case should be built and executed first and then built 
    // and executed the testSender() in "../client/clientTest.cc"
    Receiver receiver;
    while(1)
    {
        int receivedSymN = 0;
        int limit = 100;
        string filename;
        vector<symbol> symbols(limit);
        struct symbol* symP;
        for(int i = 0; i < limit; ++i)
        {
            symP = receiver.getSymbol();
            ++receivedSymN;
            filename = string(symP->filenameArray);
            cout << "filename : " << filename << "; receive "
                << receivedSymN << endl;
            *(symbols.begin() + i) = *symP;
        }
        receiver.endIteration();
    }
}

void testDecoder()
{
    // this test case should be built and executed first and then built 
    // and executed the testEncoder() in "../client/clientTest.cc"
    string savePath("../../data/receiveData/");
    Decoder decoder(savePath);
    decoder.start();
    return;
}

int main()
{
    testDecoder();
    // testReceiver();
    return 0;
}

