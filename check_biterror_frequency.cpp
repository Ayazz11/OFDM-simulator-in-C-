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
#include <algorithm>
using namespace std;
using Complex = complex <float>;

void printBits(const vector<uint8_t>& bits, size_t start, size_t end)
{
    for (size_t i = start; i < end; ++i)
    {
        cout << static_cast<int>(bits[i]);
    }
}

double printChunkErrors(
    const vector<uint8_t>& transmittedBits,
    const vector<uint8_t>& recoveredBits,
    size_t frameNumber)
{
    constexpr size_t chunkSize = 11;
    size_t frameErrors = 0;
    size_t numberOfChunks = 0;

    cout << "\nFrame " << frameNumber << " chunk comparison:\n";
    for (size_t start = 0; start < transmittedBits.size(); start += chunkSize)
    {
        const size_t end = min(start + chunkSize, transmittedBits.size());
        size_t flippedBits = 0;

        for (size_t i = start; i < end; ++i)
        {
            if (transmittedBits[i] != recoveredBits[i])
            {
                ++flippedBits;
            }
        }

        frameErrors += flippedBits;
        ++numberOfChunks;
                cout << "Chunk " << numberOfChunks << " (" << end - start
             << " bits)  TX: ";
        printBits(transmittedBits, start, end);
        cout << "  RX: ";
        printBits(recoveredBits, start, end);
        cout << "  flipped: " << flippedBits << '\n';
    }
    const double averageFlipsPerChunk =
        static_cast<double>(frameErrors) / numberOfChunks;
    cout << "Frame " << frameNumber << " errors: " << frameErrors << '\n'
         << "Average flipped bits per chunk: "
         << averageFlipsPerChunk << '\n';
    return averageFlipsPerChunk;
}

int main(){
    SystemConfig config;
    BitSource b1(42);
    size_t bps=config.bitsPerSymbol; // number of bits per symbol.
    size_t n=config.bitsPerSymbol*config.numUsedSubcarriers;; // number of bits per OFDM symbol.
    QamModulator m1(bps);
    OfdmModulator ofdm_mod1(config);
    Channel awgn(82);
    // Estimate BER over many noisy repetitions. One frame is too small to
    // provide a useful BER estimate at this SNR.
    const int numFrames = 1000;
    int error_bits = 0;
    for (int frame = 0; frame < numFrames; ++frame)
    {
        // Generate and modulate a new frame of n bits for every iteration.
        // The bit source remains outside the loop, so its RNG state advances.
        vector <uint8_t> frame_bits = b1.generate(n);
        vector <Complex> frame_qam_symbols = m1.modulate(frame_bits);
        vector <Complex> frame_tx_signal = ofdm_mod1.modulate(frame_qam_symbols);
        vector <Complex> rx_time_signal_awgn = awgn.addAwgn(frame_tx_signal, 5, bps);
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

        // Print one complete frame without producing 10,000 lines of output.
        //if(frame==0){
        //printChunkErrors(frame_bits, recovered_bits_awgn, frame + 1);
        //}
    }
    const double total_bits = static_cast<double>(n) * numFrames;
    cout<<"Simulated BER :" << error_bits / total_bits << endl;
    double theoreticalBer = BerAnalyzer::theoreticalBerAWGN(5, bps);
    cout<<"theoretical BER :" <<theoreticalBer<<endl;
    cout<<"error bits for the entire frame :" << error_bits<<endl;
    return 0;
}