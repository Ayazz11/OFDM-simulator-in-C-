#include "SystemConfig.hpp"
#include "LinkSimulator.hpp"

#include <iostream>
#include <filesystem>

int main()
{
    // --------------------------------------------------------
    // 1. Create system configuration
    // --------------------------------------------------------

    SystemConfig config;


    // --------------------------------------------------------
    // 2. Display the current configuration
    // --------------------------------------------------------

    std::cout << "===== OFDM Link Simulator =====\n\n";

    std::cout << "FFT size:         "
              << config.fftSize << "\n";

    std::cout << "Used subcarriers: "
              << config.numUsedSubcarriers << "\n";

    std::cout << "CP length:        "
              << config.cpLen << "\n";

    std::cout << "Bits per symbol:  "
              << config.bitsPerSymbol << "\n";

    std::cout << "SNR range:        "
              << config.snrStartDb << " to "
              << config.snrStopDb << " dB\n\n";


    // --------------------------------------------------------
    // 3. Create the OFDM link simulator
    // --------------------------------------------------------

    LinkSimulator simulator(config);


    // --------------------------------------------------------
    // 4. Test the system without noise
    // --------------------------------------------------------

    std::cout << "Running loopback test...\n";

    bool testPassed = simulator.loopbackTest();

    if (!testPassed)
    {
        std::cout << "Loopback test FAILED.\n";
        std::cout << "Please check the OFDM transmitter/receiver.\n";

        return 1;
    }

    std::cout << "Loopback test PASSED.\n\n";


    // --------------------------------------------------------
    // 5. Run BER simulation
    // --------------------------------------------------------

    std::cout << "Running BER simulation...\n\n";

    std::vector<BerResult> results =
        simulator.runBerSimulation();


    // --------------------------------------------------------
    // 6. Create output directory
    // --------------------------------------------------------

    std::filesystem::path outputPath(config.resultsCsvPath);

    if (!outputPath.parent_path().empty())
    {
        std::filesystem::create_directories(
            outputPath.parent_path()
        );
    }


    // --------------------------------------------------------
    // 7. Save results
    // --------------------------------------------------------

    LinkSimulator::saveResults(
        config.resultsCsvPath,
        results
    );

    std::cout << "\nSimulation completed.\n";
    std::cout << "Results saved to: "
              << config.resultsCsvPath << "\n";


    return 0;
}