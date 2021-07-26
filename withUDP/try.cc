#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <iterator>
#include <algorithm>
#include <unordered_set>

using namespace std;
 
int main () {
    std::vector<int> a = {1, 2};
    std::vector<std::unordered_set<int>> uset;
    uset.resize(2);
    std::copy(a.begin(), a.end(), std::inserter(uset[0], uset[0].end()));
    for(int i : a) 
        cout << i << endl;
  return 0;
}
