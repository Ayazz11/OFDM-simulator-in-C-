# include "BitSource.hpp"
#include <cstdint>
#include <vector>
#include <iostream>
using namespace std;
int main(){
    BitSource b1(42);
    vector <uint8_t> bits = b1.generate(100);
    //cout<<sizeof(int)<<" "<<sizeof(unsigned)<<" "<<sizeof(size_t)<<" "<<sizeof(uint8_t)<<endl;
    for(auto x: bits){
        cout<<int(x)<<" ";
    }
    return 0;
}