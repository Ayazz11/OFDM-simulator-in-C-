#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "OfdmModulator.hpp"
#include "SystemConfig.hpp"
#include <random>

TEST_CASE("OFDM modulate/demodulate round-trip is bit-exact (no channel)", "[ofdm]")
{
    SystemConfig cfg;
    OfdmModulator ofdm(cfg);

    std::mt19937 rng(11);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);

    std::vector<cf32> freqSymbols(cfg.numUsedSubcarriers);
    for (auto& s : freqSymbols) s = cf32(d(rng), d(rng));

    auto waveform = ofdm.modulate(freqSymbols);
    REQUIRE(waveform.size() == cfg.fftSize + cfg.cpLen);

    auto rxSymbols = ofdm.demodulate(waveform);
    REQUIRE(rxSymbols.size() == freqSymbols.size());

    for (std::size_t i = 0; i < freqSymbols.size(); ++i)
    {
        REQUIRE(rxSymbols[i].real() == Catch::Approx(freqSymbols[i].real()).margin(1e-4));
        REQUIRE(rxSymbols[i].imag() == Catch::Approx(freqSymbols[i].imag()).margin(1e-4));
    }
}

TEST_CASE("Cyclic prefix is a copy of the tail of the OFDM symbol", "[ofdm]")
{
    SystemConfig cfg;
    OfdmModulator ofdm(cfg);

    std::mt19937 rng(12);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    std::vector<cf32> freqSymbols(cfg.numUsedSubcarriers);
    for (auto& s : freqSymbols) s = cf32(d(rng), d(rng));

    auto waveform = ofdm.modulate(freqSymbols);

    // First cpLen samples of waveform must equal the last cpLen samples of
    // the (cpLen..end) body -- i.e. body[body.size()-cpLen .. end) == prefix.
    for (std::size_t i = 0; i < cfg.cpLen; ++i)
    {
        cf32 prefixSample = waveform[i];
        cf32 tailSample = waveform[cfg.cpLen + cfg.fftSize - cfg.cpLen + i];
        REQUIRE(prefixSample.real() == Catch::Approx(tailSample.real()));
        REQUIRE(prefixSample.imag() == Catch::Approx(tailSample.imag()));
    }
}

TEST_CASE("Subcarrier mapping leaves DC (bin 0) unused", "[ofdm]")
{
    SystemConfig cfg;
    OfdmModulator ofdm(cfg);
    for (auto idx : ofdm.subcarrierIndices())
        REQUIRE(idx != 0);
}

TEST_CASE("Energy is preserved across IFFT/FFT (Parseval, unitary scaling)", "[ofdm]")
{
    SystemConfig cfg;
    OfdmModulator ofdm(cfg);

    std::mt19937 rng(13);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    std::vector<cf32> freqSymbols(cfg.numUsedSubcarriers);
    for (auto& s : freqSymbols) s = cf32(d(rng), d(rng));

    double freqEnergy = 0.0;
    for (auto& s : freqSymbols) freqEnergy += std::norm(s);

    auto waveform = ofdm.modulate(freqSymbols);
    // Only the fftSize body carries "new" energy comparable to freqEnergy
    // (the CP duplicates part of it, so exclude it from this comparison).
    double bodyEnergy = 0.0;
    for (std::size_t i = cfg.cpLen; i < waveform.size(); ++i)
        bodyEnergy += std::norm(waveform[i]);

    REQUIRE(bodyEnergy == Catch::Approx(freqEnergy).margin(1e-3));
}
