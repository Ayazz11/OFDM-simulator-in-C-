#pragma once
#include <cstddef>
#include <string>

class SystemConfig
{
    public : 
    // --- OFDM / numerology parameters ---
    size_t fftSize        = 64;   // N-point IFFT/FFT size
    size_t numUsedSubcarriers = 52;   // active (data-bearing) subcarriers
    size_t cpLen          = 16;   // cyclic prefix length in samples
    // --- Modulation ---
    // Bits per symbol: 2 = QPSK, 4 = 16-QAM, 6 = 64-QAM
    size_t bitsPerSymbol  = 4;

    // --- Simulation control ---
    size_t numOfdmSymbolsPerTrial = 100;   // OFDM symbols per Monte Carlo trial
    size_t numTrialsPerSnr        = 200;   // independent trials averaged per SNR point
    double      snrStartDb            = 0.0;
    double      snrStopDb              = 12.0;
    double      snrStepDb            = 1.0;
    unsigned int    randomSeed     = 42;   // fixed seed -> reproducible results
    std::string results_awgn_CsvPath = "results_awgn/ber_results.csv";
    // Derived quantity: fraction of samples that are "overhead" (CP), useful
    // for reporting spectral efficiency in the analysis stage.
    double cpOverheadFraction() const
    {
        return static_cast<double>(cpLen) /
               static_cast<double>(fftSize + cpLen);
    }
};
