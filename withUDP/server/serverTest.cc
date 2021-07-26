#include <iostream>
#include "globalParameters.h"

using namespace std;

void testReceiver()
{
    return;
}

void testDecoder()
{
    string savePath("../../data/receiveData/");
    Decoder decoder(savePath);
    decoder.start();
    return;
}

int main()
{
    return 0;
}

