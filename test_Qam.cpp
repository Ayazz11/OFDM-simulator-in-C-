# include "BitSource.hpp"
#include <cstdint>
#include <vector>
#include <iostream>
#include <complex>
#include "QamModulator.hpp"
using namespace std;
using Complex = complex <float>;
int main(){
    size_t bps=4;
    BitSource b1(42);
    QamModulator m1(bps);
    vector <uint8_t> bits = b1.generate(100);
    size_t qam_size = bits.size() / bps;
    vector <Complex> modulated_symbols(qam_size);
    vector <uint8_t> demodulated_bits(100);
    modulated_symbols=m1.modulate(bits);
    cout<<endl<<endl;
    for(size_t i = 0; i < qam_size; i++)
    {
        size_t start = i * bps;
        for(size_t j = start; j < start + bps; j++)
            {
                cout << int(bits[j]);
            }
        cout << " -> " << modulated_symbols[i] << endl;
    }
    demodulated_bits = m1.demodulate(modulated_symbols);
    for(auto x: bits){
        cout<<int(x);
    }
    cout<<endl;
    for(auto x: demodulated_bits){
        cout<<int(x);
    }
    if(bits==demodulated_bits){
        cout<<endl<<"Modulation and Demodulation working properly";
    }
    return 0;
}