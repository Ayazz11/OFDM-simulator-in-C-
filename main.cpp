#include "SystemConfig.hpp"
#include "LinkSimulator.hpp"
#include <iostream>
#include <filesystem>

int main()
{
    SystemConfig config;
    std::cout << "===== OFDM Link Simulator =====\n\n";
    std::cout << "FFT size: "<< config.fftSize << "\n";
    std::cout << "Used subcarriers: "<< config.numUsedSubcarriers << "\n";
    std::cout << "CP length: "<< config.cpLen << "\n";
    std::cout << "Bits per symbol:  "<< config.bitsPerSymbol << "\n";
    std::cout << "SNR range: "<< config.snrStartDb << " to "<< config.snrStopDb << " dB\n\n";
    LinkSimulator simulator(config);
    std::cout << "Running loopback test...\n";
    bool testPassed = simulator.sanityCheckLoopback();
    if (!testPassed)
    {
        std::cout << "Loopback test FAILED.\n";
        std::cout << "Please check the OFDM transmitter/receiver.\n";
        return 1;
    }
    std::cout << "Loopback test PASSED.\n\n";
    std::cout << "Running BER simulation...\n\n";
    std::vector<BerResultPoint> results = simulator.runBerSweep();

    std::filesystem::path outputPath(config.results_awgn_CsvPath);
    if (!outputPath.parent_path().empty())
    {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    LinkSimulator::writeCsv(
        config.results_awgn_CsvPath,
        results
    );
    std::cout << "\nSimulation completed.\n";
    std::cout << "Results saved to: "
              << config.results_awgn_CsvPath << "\n";
    return 0;
}