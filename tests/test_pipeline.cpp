#include <catch2/catch_test_macros.hpp>
#include "LinkSimulator.hpp"
#include "BerAnalyzer.hpp"

TEST_CASE("Full pipeline bit-perfect loopback with no channel", "[pipeline]")
{
    SystemConfig cfg;
    cfg.numOfdmSymbolsPerTrial = 50;
    LinkSimulator sim(cfg);
    REQUIRE(sim.sanityCheckLoopback());
}

TEST_CASE("Full pipeline loopback holds for 16-QAM and 64-QAM too", "[pipeline]")
{
    for (std::size_t bps : {2, 4, 6})
    {
        SystemConfig cfg;
        cfg.bitsPerSymbol = bps;
        cfg.numOfdmSymbolsPerTrial = 50;
        LinkSimulator sim(cfg);
        REQUIRE(sim.sanityCheckLoopback());
    }
}

TEST_CASE("Simulated BER tracks the theoretical AWGN curve at high SNR", "[pipeline][ber]")
{
    // At high Eb/N0 the simulated BER should be close (same order of
    // magnitude / within a reasonable multiplicative factor) to the
    // closed-form theoretical BER -- this is the core correctness claim
    // of the whole simulator.
    SystemConfig cfg;
    cfg.bitsPerSymbol = 2; // QPSK
    cfg.snrStartDb = 8.0;
    cfg.snrStopDb = 8.0;
    cfg.snrStepDb = 1.0;
    cfg.numOfdmSymbolsPerTrial = 200;
    cfg.numTrialsPerSnr = 100;

    LinkSimulator sim(cfg);
    auto results = sim.runBerSweep();
    REQUIRE(results.size() == 1);

    const double simBer = results[0].simulatedBer;
    const double theoryBer = results[0].theoreticalBer;

    // Within 3x of theory (Monte Carlo noise at ~1e-4 BER with this many
    // bits gives a reasonably tight but not exact match).
    REQUIRE(simBer < theoryBer * 3.0);
    REQUIRE(simBer > theoryBer / 3.0);
}
