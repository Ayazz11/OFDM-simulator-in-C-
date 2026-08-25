#pragma once
#include <cmath>
#include <cstddef>

// ---------------------------------------------------------------------------
// BerAnalyzer
//
// Closed-form theoretical bit error rate for Gray-coded square M-QAM in
// AWGN, used as the ground truth the Monte Carlo simulation is validated
// against. Standard approximate result (exact for QPSK, very close
// approximation -- ignoring rare multi-bit-per-symbol error events -- for
// 16/64-QAM):
//
//   BER ~= (4 / log2(M)) * (1 - 1/sqrt(M)) * Q( sqrt(3*log2(M)/(M-1) * Eb/N0) )
//
// where Q(x) = 0.5 * erfc(x / sqrt(2)).
// ---------------------------------------------------------------------------
namespace BerAnalyzer
{
    inline double qFunction(double x)
    {
        return 0.5 * std::erfc(x / std::sqrt(2.0));
    }

    // ebN0Db: Eb/N0 in dB. bitsPerSymbol: 2 (QPSK), 4 (16-QAM), 6 (64-QAM).
    inline double theoreticalBer(double ebN0Db, std::size_t bitsPerSymbol)
    {
        const double ebN0Linear = std::pow(10.0, ebN0Db / 10.0);
        const double M = static_cast<double>(1ull << bitsPerSymbol);
        const double k = static_cast<double>(bitsPerSymbol);

        const double argInside = 3.0 * k / (M - 1.0) * ebN0Linear;
        const double prefactor = (4.0 / k) * (1.0 - 1.0 / std::sqrt(M));

        return prefactor * qFunction(std::sqrt(argInside));
    }
}
