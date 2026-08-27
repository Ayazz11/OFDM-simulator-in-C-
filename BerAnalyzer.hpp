#pragma once
#include <cmath>
#include <cstddef>

namespace BerAnalyzer
{
    // Q-function
    double qFunction(double x)
    {
        return 0.5 * std::erfc(x / std::sqrt(2.0));
    }

    double theoreticalBerAWGN( double ebN0Db, std::size_t bitsPerSymbol)
    {
        // Convert Eb/N0 from dB to linear
        double ebN0 = std::pow(10.0, ebN0Db / 10.0);
        // M = 2^k
        double M = std::pow(2.0, bitsPerSymbol);
        // Argument of Q-function
        double argument = std::sqrt((3.0 * bitsPerSymbol * ebN0)/ (M - 1.0));
        // Prefactor
        double factor = (4.0 / bitsPerSymbol) * (1.0 - 1.0 / std::sqrt(M));
        return factor * qFunction(argument);
    }
}