#pragma once
#include "SystemConfig.hpp"
#include <vector>
#include <string>

// One row of the BER-vs-SNR sweep result.
struct BerResultPoint
{
    double snrDb;
    double simulatedBer;
    double theoreticalBer;
    std::size_t totalBitsSimulated;
    std::size_t totalBitErrors;
};

// ---------------------------------------------------------------------------
// LinkSimulator
//
// Wires BitSource -> QamModulator -> OfdmModulator -> Channel ->
// OfdmModulator (demod) -> QamModulator (demap) into a single Monte Carlo
// BER-vs-SNR sweep, per SystemConfig. This is the "pipeline orchestration"
// layer -- it owns no DSP logic itself, only sequencing and accumulation.
// ---------------------------------------------------------------------------
class LinkSimulator
{
public:
    explicit LinkSimulator(const SystemConfig& cfg) : config(cfg) {}

    // Runs one loopback pass (no channel) -- returns true iff
    // every bit round-trips correctly.
    bool loopbackTest() const;
    // Runs the full Monte Carlo sweep across [snrStartDb, snrStopDb] in
    // steps of snrStepDb and returns one BerResultPoint per SNR value.
    std::vector<BerResultPoint> runBerSweep() const;

    // Writes the sweep results to a CSV file (snr_db,simulated_ber,theoretical_ber,...).
    static void writeCsv(const std::string& path, const std::vector<BerResultPoint>& results);

private:
    SystemConfig config;
};
