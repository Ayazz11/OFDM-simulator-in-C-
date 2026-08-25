#pragma once
#include <vector>
#include <complex>
#include <random>
#include <cmath>

using cf32 = std::complex<float>;

// ---------------------------------------------------------------------------
// Channel
//
// AWGN channel applied in the time domain (i.e. to the CP-included waveform,
// exactly as noise would enter a real receiver front-end before CP removal).
//
// Modeling note / documented assumption:
//   Noise variance is derived from Eb/N0 assuming the *information* symbol
//   energy (Es, normalized to 1 at the QAM mapper) is what defines Eb, i.e.
//   Eb = Es / bitsPerSymbol. Because the IFFT/FFT pair here is unitary
//   (energy-preserving) and the same complex-noise variance added per time
//   sample reappears as the *same* variance per frequency-domain subcarrier
//   after the FFT (a standard property of the DFT of white noise), adding
//   noise with variance N0 per time sample is equivalent to adding variance
//   N0 per subcarrier at the symbol level -- which is what makes the
//   resulting BER curve directly comparable to the closed-form AWGN bound.
//
//   This deliberately does NOT inflate N0 to account for the extra transmit
//   energy spent on the cyclic prefix (a real transmitter pays a small SNR
//   penalty of roughly cpLen/(fftSize+cpLen) in dB for that redundancy).
//   That is a known, explicitly-flagged simplification -- see
//   SystemConfig::cpOverheadFraction(), which is reported separately in the
//   analysis stage rather than folded into the noise model.
// ---------------------------------------------------------------------------
class Channel
{
public:
    explicit Channel(unsigned seed) : rng_(seed), normal_(0.0f, 1.0f) {}

    // Adds complex AWGN with E[|n|^2] = N0 to every sample of `waveform`,
    // where N0 is derived from ebN0Db assuming unit average symbol energy.
    std::vector<cf32> addAwgn(const std::vector<cf32>& waveform,double ebN0Db,std::size_t bitsPerSymbol)
    {
        const double ebN0Linear = std::pow(10.0, ebN0Db / 10.0);
        const double es = 1.0; // QamModulator normalizes average symbol energy to 1
        const double n0 = es / (static_cast<double>(bitsPerSymbol) * ebN0Linear);
        const float sigmaPerDim = static_cast<float>(std::sqrt(n0 / 2.0));

        std::vector<cf32> out(waveform.size());
        for (std::size_t i = 0; i < waveform.size(); ++i)
        {
            const cf32 noise(sigmaPerDim * normal_(rng_), sigmaPerDim * normal_(rng_));
            out[i] = waveform[i] + noise;
        }
        return out;
    }

private:
    std::mt19937 rng_;
    std::normal_distribution<float> normal_;
};
