#pragma once
#include "SystemConfig.hpp"
#include <vector>
#include <string>
#include "LinkSimulator.hpp"
#include "BitSource.hpp"
#include "SystemConfig.hpp"
#include "QamModulator.hpp"
#include "OfdmModulator.hpp"
#include "Channel.hpp"
#include "BerAnalyzer.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
// One row of the BER-vs-SNR sweep result.
struct BerResultPoint
{
    double snrDb;
    double simulatedBer;
    double theoreticalBer;
    std::size_t totalBitsSimulated;
    std::size_t totalBitErrors;
};

class LinkSimulator
{
public:
    LinkSimulator(const SystemConfig& cfg) : config(cfg) {}
    // Runs one loopback pass (no channel) -- returns true iff
    // every bit round-trips correctly.
    bool sanityCheckLoopback() const {
    // Create the different blocks of our OFDM system.
    BitSource bitSource(config.randomSeed);
    QamModulator qam(config.bitsPerSymbol);
    OfdmModulator ofdm(config);

    // Number of bits carried by one OFDM symbol.
    // Example:
    // 52 subcarriers × 2 bits/subcarrier = 104 bits
    std::size_t bitsPerOfdmSymbol = config.numUsedSubcarriers * config.bitsPerSymbol;

    // Generate all bits needed for this test.
    std::size_t totalBits = bitsPerOfdmSymbol * config.numOfdmSymbolsPerTrial;
    std::vector <uint8_t> transmittedBits = bitSource.generate(totalBits);

    // Convert all bits into QAM symbols.
    std::vector<Complex> qamSymbols = qam.modulate(transmittedBits);

    // This vector will store the bits recovered by the receiver.
    std::vector<uint8_t> receivedBits;
    receivedBits.reserve(transmittedBits.size());

    // Process one OFDM symbol at a time.
    for (std::size_t symbol = 0;symbol < config.numOfdmSymbolsPerTrial; symbol++)
    {
        // Find which QAM symbols belong to this OFDM symbol.
        std::size_t start = symbol * config.numUsedSubcarriers;
        std::size_t end = start + config.numUsedSubcarriers;
        std::vector<Complex> frequencySymbols(qamSymbols.begin() + start, qamSymbols.begin() + end);
        // Transmitter :
        // QAM symbols → OFDM waveform
        std::vector<Complex> transmittedWaveform = ofdm.modulate(frequencySymbols);

        // No channel/noise.
        // Directly send the waveform to the receiver.
        std::vector<Complex> receivedFrequencySymbols =  ofdm.demodulate(transmittedWaveform);

        // Receiver:
        // QAM symbols → bits
        std::vector<uint8_t> receivedSymbolBits = qam.demodulate(receivedFrequencySymbols);
        // Add these bits to the complete received bit vector.
        for (uint8_t bit : receivedSymbolBits)
        {
            receivedBits.push_back(bit);
        }
    }
    // Compare transmitted and received bits.
    return (transmittedBits == receivedBits);
    };

    
    // Runs the full Monte Carlo sweep across [snrStartDb, snrStopDb] in
    // steps of snrStepDb and returns one BerResultPoint per SNR value.
    std::vector<BerResultPoint> runBerSweep() const {
        // Create the different system blocks.
    BitSource bitSource(config.randomSeed);
    QamModulator qam(config.bitsPerSymbol);
    OfdmModulator ofdm(config);
    Channel channel_awgn(config.randomSeed + 1);

    std::size_t bitsPerOfdmSymbol = config.numUsedSubcarriers * config.bitsPerSymbol;
    // Store the result for every SNR value.
    std::vector<BerResultPoint> results;
    //snr loop
    for (double snrDb = config.snrStartDb; snrDb <= config.snrStopDb; snrDb += config.snrStepDb)
    {
        // Count errors and transmitted bits for this SNR.
        std::size_t totalBits = 0;
        std::size_t totalErrors = 0;

        // Run several independent trials.
        for (std::size_t trial = 0; trial < config.numTrialsPerSnr; trial++)
        {

            std::size_t numberOfBits = bitsPerOfdmSymbol * config.numOfdmSymbolsPerTrial;
            std::vector<uint8_t> transmittedBits = bitSource.generate(numberOfBits);
            
            // Bits → QAM symbols
            std::vector<Complex> qamSymbols = qam.modulate(transmittedBits);
            // Store all bits recovered from this trial.
            std::vector<uint8_t> receivedBits;
            receivedBits.reserve(transmittedBits.size());

            // Process each OFDM symbol.
            for (std::size_t symbol = 0; symbol < config.numOfdmSymbolsPerTrial; symbol++)
            {
                // Find the QAM symbols belonging to this OFDM symbol.
                std::size_t start = symbol * config.numUsedSubcarriers;
                std::size_t end = start + config.numUsedSubcarriers;
                std::vector<Complex> frequencySymbols( qamSymbols.begin() + start, qamSymbols.begin() + end);
                std::vector<Complex> transmittedWaveform = ofdm.modulate(frequencySymbols);
                std::vector<Complex> receivedWaveform = channel_awgn.addAwgn(
                    transmittedWaveform,
                    snrDb,
                    config.bitsPerSymbol);
                std::vector<Complex> receivedFrequencySymbols = ofdm.demodulate(receivedWaveform);
                std::vector<uint8_t> receivedSymbolBits = qam.demodulate(receivedFrequencySymbols);

                for (uint8_t bit : receivedSymbolBits)
                {
                    receivedBits.push_back(bit);
                }
            }

            // we simply count the error bits in this trial and the total transmitted bits, repeat this for every trail
            // for every trial totalErrors and totalBits is getting updated.
            // recievedBits is new for every trial.
            for (std::size_t i = 0; i < transmittedBits.size(); i++)
            {
                if (transmittedBits[i] != receivedBits[i])
                {
                    totalErrors++;
                }
            }

            // Add number of transmitted bits to the total.
            totalBits += transmittedBits.size();
        }


        // Calculate BER for this SNR.
        double simulatedBer = static_cast<double>(totalErrors) / static_cast<double>(totalBits);
        // Calculate theoretical BER.
        double theoreticalBer = BerAnalyzer::theoreticalBerAWGN(snrDb, config.bitsPerSymbol);

        // Store all results for this SNR.
        BerResultPoint result;

        result.snrDb = snrDb;
        result.simulatedBer = simulatedBer;
        result.theoreticalBer = theoreticalBer;
        result.totalBitsSimulated = totalBits;
        result.totalBitErrors = totalErrors;
        results.push_back(result);

        // Print progress on the terminal.
        std::cout<< "SNR = " << snrDb << " dB"
            << " | Simulated BER = " << simulatedBer
            << " | Theoretical BER = " << theoreticalBer
            << " | Errors = " << totalErrors
            << " / " << totalBits
            << std::endl;
    }

    return results;
    };

    // Writes the sweep results to a CSV file (snr_db,simulated_ber,theoretical_ber,...).
    static void writeCsv(const std::string& filePath, const std::vector<BerResultPoint>& results){
        
    // Open the file.
    std::ofstream file(filePath);

    // Check whether the file was opened successfully.
    if (!file)
    {
        throw std::runtime_error(
            "Could not open file: " + filePath
        );
    }
    // Write the CSV header.
    file << "snr_db,"
         << "simulated_ber,"
         << "theoretical_ber,"
         << "total_bits,"
         << "bit_errors\n";

    // Write one row for every SNR result.
    for (const BerResultPoint& result : results)
    {
        file << result.snrDb << ","
             << result.simulatedBer << ","
             << result.theoreticalBer << ","
             << result.totalBitsSimulated << ","
             << result.totalBitErrors
             << "\n";
    }
    };

private:
    SystemConfig config;
};
