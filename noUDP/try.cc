#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <iterator>
#include <algorithm>
#include <unordered_set>
#include <math.h>

using namespace std;
 
int main () {
    discrete_distribution<long> ranD(10);
    ranD[1] = 1;
    cout << ranD[1] << endl;
  return 0;
}
