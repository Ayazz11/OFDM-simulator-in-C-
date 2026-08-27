#pragma once
#include <vector>
#include <complex>
#include <random>
#include <cmath>
using cf32 = std::complex<float>;

class Channel{
public:
     Channel(unsigned seed) : rng(seed), normal(0.0f, 1.0f) {}

    // Adds complex AWGN with E[|n|^2] = N0 to every sample of waveform,
    // where N0 is derived from ebN0Db assuming unit average symbol energy.
    std::vector<cf32> addAwgn(
        const std::vector<cf32>& waveform,
        double ebN0Db,
        std::size_t bitsPerSymbol)
    {
        const double ebN0Linear = std::pow(10.0, ebN0Db / 10.0);
        const double es = 1.0; // QamModulator normalizes average symbol energy to 1
        const double qamN0 = es / (static_cast<double>(bitsPerSymbol) * ebN0Linear);
        const float sigmaPerDim = static_cast<float>(std::sqrt(qamN0 / 2.0));

        std::vector<cf32> out(waveform.size());
        for (std::size_t i = 0; i < waveform.size(); ++i)
        {
            const cf32 noise(sigmaPerDim * normal(rng), sigmaPerDim * normal(rng));
            out[i] = waveform[i] + noise;
        }
        return out;
    }

private:
    std::mt19937 rng;
    std::normal_distribution<float> normal;
};
