
#include <iostream>
#include <vector>
#include "./Receiver.h"
#include "./globalParameters.h"

using namespace std;

class A
{
    public:
        A();
    private:
        Receiver r;
};

int main()
{
    cout << SER_BUF_SIZE << endl;
    return 0;
}
