#include "SystemConfig.hpp"
#include "LinkSimulator2.hpp"
#include <filesystem>
#include <iostream>

int main()
{
    SystemConfig config;
    config.bitsPerSymbol = 4;
    config.numUsedSubcarriers = 52;
    config.numTrialsPerSnr = 200;
    config.numOfdmSymbolsPerTrial = 100;
    config.snrStartDb = 0.0;
    config.snrStopDb = 12.0;
    config.snrStepDb = 1.0;
    config.randomSeed = 42;

    std::cout << "===== Hamming-Coded OFDM Link Simulator =====\n\n";
    std::cout << "FFT size: " << config.fftSize << "\n";
    std::cout << "Used subcarriers: " << config.numUsedSubcarriers << "\n";
    std::cout << "CP length: " << config.cpLen << "\n";
    std::cout << "Bits per symbol: " << config.bitsPerSymbol << "\n";
    std::cout << "SNR range: " << config.snrStartDb << " to " << config.snrStopDb << " dB\n\n";

    LinkSimulator2 simulator(config);

    std::cout << "Running loopback test...\n";
    bool testPassed = simulator.sanityCheckLoopback();
    if (!testPassed)
    {
        std::cout << "Loopback test FAILED.\n";
        std::cout << "Please check the Hamming-coded OFDM pipeline.\n";
        return 1;
    }
    std::cout << "Loopback test PASSED.\n\n";

    std::cout << "Running BER simulation...\n\n";
    std::vector<BerResultPoint> results = simulator.runBerSweep();

    const std::string outputPath = "results_awgn/ber_results_hamming.csv";
    std::filesystem::path csvPath(outputPath);
    if (!csvPath.parent_path().empty())
    {
        std::filesystem::create_directories(csvPath.parent_path());
    }

    LinkSimulator2::writeCsv(outputPath, results);

    std::cout << "\nSimulation completed.\n";
    std::cout << "Results saved to: " << outputPath << "\n";
    return 0;
}
