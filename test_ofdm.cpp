# include "BitSource.hpp"
#include "SystemConfig.hpp"
#include <cstdint>
#include <vector>
#include <iostream>
#include <complex>
#include "QamModulator.hpp"
#include "OfdmModulator.hpp"
#include "Channel.hpp"
#include "BerAnalyzer.hpp"
using namespace std;
using Complex = complex <float>;
int main(){
    size_t bps=4;
    size_t n=208;
    SystemConfig config;
    BitSource b1(42);
    QamModulator m1(bps);
    OfdmModulator ofdm_mod1(config);
    Channel awgn(82);
    vector <uint8_t> bits = b1.generate(n);
    size_t qam_size = bits.size() / bps;
    vector <Complex> qam_modulated_symbols(qam_size);
    qam_modulated_symbols=m1.modulate(bits);
    vector <uint8_t> demodulated_bits(n);
    demodulated_bits = m1.demodulate(qam_modulated_symbols);
    if(bits==demodulated_bits){
        cout<<endl<<"Modulation and Demodulation working properly";
    }
    cout<<endl<<qam_size;
    vector <Complex> trx_time_signal = ofdm_mod1.modulate(qam_modulated_symbols);
    cout<<endl;
    cout<<trx_time_signal.size()<<endl;
    ///for(auto x: trx_time_signal){cout<<x<<" ";}
    vector <Complex> demodulate_trx_time_signal = ofdm_mod1.demodulate(trx_time_signal);
    cout<<demodulate_trx_time_signal.size()<<endl;
    for(int i=0;i<52;i++){
        cout<<qam_modulated_symbols[i]<<"->"<<demodulate_trx_time_signal[i]<<endl;
    }
        bool ofdmWorks = true;
    float tolerance = 1e-4f;
    for (size_t i = 0; i < qam_modulated_symbols.size(); i++)
    {
        float error = abs(qam_modulated_symbols[i] - demodulate_trx_time_signal[i]);
        if (error > tolerance)
        {
            ofdmWorks = false;
            cout << "Error at subcarrier"<<i<< " = "<<error<< endl;
        }
    }
    if (ofdmWorks)
        cout << "OFDM test: PASS\n";
    else
        cout << "OFDM test: FAIL\n";
    // Estimate BER over many noisy repetitions. One frame is too small to
    // provide a useful BER estimate at this SNR.
    const int numFrames = 10000;
    int error_bits = 0;
    for (int frame = 0; frame < numFrames; ++frame)
    {
        // Generate and modulate a new frame for every iteration. The bit
        // source remains outside the loop, so its RNG state advances.
        vector <uint8_t> frame_bits = b1.generate(n);
        vector <Complex> frame_qam_symbols = m1.modulate(frame_bits);
        vector <Complex> frame_tx_signal = ofdm_mod1.modulate(frame_qam_symbols);

        vector <Complex> rx_time_signal_awgn = awgn.addAwgn(frame_tx_signal, 10, bps);
        vector <Complex> recovered_qam_symbols_awgn = ofdm_mod1.demodulate(rx_time_signal_awgn);
        vector <uint8_t> recovered_bits_awgn = m1.demodulate(recovered_qam_symbols_awgn);
        
        int frame_errors = 0;
        for (size_t i = 0; i < n; i++)
        {
            if (frame_bits[i] != recovered_bits_awgn[i])
            {
                ++frame_errors;
            }
        }
        error_bits += frame_errors;
    }
    const double total_bits = static_cast<double>(n) * numFrames;
    cout<<"Simulated BER :" << error_bits / total_bits << endl;
    double theoreticalBer = BerAnalyzer::theoreticalBerAWGN(10, 4);
    cout<<"theoretical BER :" <<theoreticalBer<<endl;
    return 0;
}