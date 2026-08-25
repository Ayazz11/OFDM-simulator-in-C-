#include "SystemConfig.hpp"
#include "LinkSimulator.hpp"
#include <iostream>
#include <filesystem>

// Runs the BER sweep for QPSK, 16-QAM, and 64-QAM and writes one CSV per
// modulation, so the analysis stage can overlay all three on one plot --
// this is the standard "waterfall" comparison plot used to report link
// performance across modulation/coding schemes.
int main()
{
    for (std::size_t bps : {2, 4, 6})
    {
        SystemConfig cfg;
        cfg.bitsPerSymbol = bps;
        cfg.snrStopDb = (bps == 2) ? 12.0 : (bps == 4) ? 22.0 : 30.0;
        cfg.snrStepDb = 2.0;
        cfg.resultsCsvPath = "results/ber_results_bps" + std::to_string(bps) + ".csv";

        std::cout << "\n=== Running bitsPerSymbol=" << bps << " ===\n";
        LinkSimulator sim(cfg);
        if (!sim.sanityCheckLoopback())
        {
            std::cerr << "Sanity check FAILED for bps=" << bps << "\n";
            continue;
        }
        auto results = sim.runBerSweep();
        std::filesystem::create_directories(
            std::filesystem::path(cfg.resultsCsvPath).parent_path());
        LinkSimulator::writeCsv(cfg.resultsCsvPath, results);
    }
    return 0;
}
