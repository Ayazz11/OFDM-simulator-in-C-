#include "LinkSimulator.hpp"

#include "BitSource.hpp"
#include "QamModulator.hpp"
#include "OfdmModulator.hpp"
#include "Channel.hpp"
#include "BerAnalyzer.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>


// ------------------------------------------------------------
// Test the complete transmitter and receiver without noise.
//
// If everything is working correctly:
//
// Bits
//   ↓
// QAM
//   ↓
// OFDM
//   ↓
// OFDM Demodulation
//   ↓
// QAM Demodulation
//   ↓
// Same bits
//
// The function returns true if all bits are recovered correctly.
// ------------------------------------------------------------
bool LinkSimulator::loopbackTest() const
{
    // Create the different blocks of our OFDM system.
    BitSource bitSource(config.randomSeed);
    QamModulator qam(config.bitsPerSymbol);
    OfdmModulator ofdm(config);


    // Number of bits carried by one OFDM symbol.
    //
    // Example:
    // 52 subcarriers × 2 bits/subcarrier = 104 bits
    //
    std::size_t bitsPerOfdmSymbol = config.numUsedSubcarriers * config.bitsPerSymbol;


    // Generate all bits needed for this test.
    std::size_t totalBits = bitsPerOfdmSymbol * config.numOfdmSymbolsPerTrial;
    std::vector<uint8_t> transmittedBits = bitSource.generate(totalBits);

    // Convert all bits into QAM symbols.
    std::vector<Complex> qamSymbols = qam.modulate(transmittedBits);

    // This vector will store the bits recovered by the receiver.
    std::vector<uint8_t> receivedBits;
    receivedBits.reserve(transmittedBits.size());

    // Process one OFDM symbol at a time.
    for (std::size_t symbol = 0;symbol < config.numOfdmSymbolsPerTrial; symbol++)
    {
        // ----------------------------------------------------
        // Find which QAM symbols belong to this OFDM symbol.
        // ----------------------------------------------------

        std::size_t start = symbol * config.numUsedSubcarriers;
        std::size_t end = start + config.numUsedSubcarriers;

        std::vector<Complex> frequencySymbols(qamSymbols.begin() + start, qamSymbols.begin() + end);

        // ----------------------------------------------------
        // Transmitter:
        //
        // QAM symbols → OFDM waveform
        // ----------------------------------------------------

        std::vector<Complex> transmittedWaveform = ofdm.modulate(frequencySymbols);

        // ----------------------------------------------------
        // No channel/noise.
        //
        // Directly send the waveform to the receiver.
        // ----------------------------------------------------

        std::vector<Complex> receivedFrequencySymbols =  ofdm.demodulate(transmittedWaveform);

        // ----------------------------------------------------
        // Receiver:
        //
        // QAM symbols → bits
        // ----------------------------------------------------

        std::vector<uint8_t> receivedSymbolBits = qam.demap(receivedFrequencySymbols);

        // Add these bits to the complete received bit vector.
        for (uint8_t bit : receivedSymbolBits)
        {
            receivedBits.push_back(bit);
        }
    }


    // Compare transmitted and received bits.
    return transmittedBits == receivedBits;
}


// ------------------------------------------------------------
// Run BER simulation for different SNR values.
//
// Example:
//
// SNR = 0 dB
// SNR = 2 dB
// SNR = 4 dB
// ...
// SNR = 20 dB
//
// At every SNR, several independent trials are performed.
// ------------------------------------------------------------
std::vector<BerResultPoint> LinkSimulator::runBerSimulation() const
{
    // Create the different system blocks.

    BitSource bitSource(config.randomSeed);
    QamModulator qam(config.bitsPerSymbol);
    OfdmModulator ofdm(config);

    // Use a different seed for noise.
    Channel channel(config.randomSeed + 1);
    // Number of bits transmitted by one OFDM symbol.

    std::size_t bitsPerOfdmSymbol = config.numUsedSubcarriers * config.bitsPerSymbol;
    // Store the result for every SNR value.
    std::vector<BerResultPoint> results;

    // --------------------------------------------------------
    // Start the SNR loop.
    // --------------------------------------------------------

    for (double snrDb = config.snrStartDb; snrDb <= config.snrStopDb; snrDb += config.snrStepDb)
    {
        // Count errors and transmitted bits for this SNR.
        std::size_t totalBits = 0;
        std::size_t totalErrors = 0;
        // ----------------------------------------------------
        // Run several independent trials.
        // ----------------------------------------------------

        for (std::size_t trial = 0; trial < config.numTrialsPerSnr; trial++)
        {
            // -----------------------------------------------
            // Generate random information bits.
            // -----------------------------------------------
            std::size_t numberOfBits = bitsPerOfdmSymbol * config.numOfdmSymbolsPerTrial;
            std::vector<uint8_t> transmittedBits = bitSource.generate(numberOfBits);
            // -----------------------------------------------
            // Bits → QAM symbols
            // -----------------------------------------------
            std::vector<Complex> qamSymbols = qam.modulate(transmittedBits);
            // Store all bits recovered from this trial.
            std::vector<uint8_t> receivedBits;
            receivedBits.reserve(transmittedBits.size());

            // -----------------------------------------------
            // Process each OFDM symbol.
            // -----------------------------------------------

            for (std::size_t symbol = 0; symbol < config.numOfdmSymbolsPerTrial; symbol++)
            {
                // Find the QAM symbols belonging to
                // this OFDM symbol.

                std::size_t start = symbol * config.numUsedSubcarriers;
                std::size_t end = start + config.numUsedSubcarriers;

                std::vector<Complex> frequencySymbols( qamSymbols.begin() + start, qamSymbols.begin() + end);


                // -------------------------------------------
                // Transmitter
                //
                // QAM symbols
                //      ↓
                // Subcarrier mapping
                //      ↓
                // IFFT
                //      ↓
                // Add CP
                //      ↓
                // Time-domain OFDM waveform
                // -------------------------------------------

                std::vector<Complex> transmittedWaveform = ofdm.modulate(frequencySymbols);

                // -------------------------------------------
                // Channel
                //
                // Add complex AWGN.
                // -------------------------------------------

                std::vector<Complex> receivedWaveform =
                    channel.addAwgn(
                        transmittedWaveform,
                        snrDb,
                        config.bitsPerSymbol
                    );


                // -------------------------------------------
                // Receiver
                //
                // Remove CP
                //      ↓
                // FFT
                //      ↓
                // Extract active subcarriers
                // -------------------------------------------

                std::vector<Complex> receivedFrequencySymbols = ofdm.demodulate(receivedWaveform);


                // -------------------------------------------
                // QAM demodulation
                //
                // QAM symbols → bits
                // -------------------------------------------

                std::vector<uint8_t> receivedSymbolBits = qam.demap(receivedFrequencySymbols);


                // Add recovered bits to the complete
                // received bit vector.

                for (uint8_t bit : receivedSymbolBits)
                {
                    receivedBits.push_back(bit);
                }
            }


            // -----------------------------------------------
            // Compare transmitted and received bits.
            // -----------------------------------------------

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


        // ----------------------------------------------------
        // Calculate BER for this SNR.
        // ----------------------------------------------------
        double simulatedBer = static_cast<double>(totalErrors) / static_cast<double>(totalBits);
        // Calculate theoretical BER.

        double theoreticalBer =
            BerAnalyzer::theoreticalBer(
                snrDb,
                config.bitsPerSymbol
            );

        // ----------------------------------------------------
        // Store all results for this SNR.
        // ----------------------------------------------------

        BerResultoint result;

        result.snrDb = snrDb;
        result.simulatedBer = simulatedBer;
        result.theoreticalBer = theoreticalBer;
        result.totalBits = totalBits;
        result.bitErrors = totalErrors;
        results.push_back(result);


        // Print progress on the terminal.

        std::cout
            << "SNR = " << snrDb << " dB"
            << " | Simulated BER = " << simulatedBer
            << " | Theoretical BER = " << theoreticalBer
            << " | Errors = " << totalErrors
            << " / " << totalBits
            << std::endl;
    }

    return results;
}


// ------------------------------------------------------------
// Save BER results to a CSV file.
// ------------------------------------------------------------
void LinkSimulator::saveResults(
    const std::string& filePath,
    const std::vector<BerResult>& results)
{
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
    for (const BerResult& result : results)
    {
        file << result.snrDb << ","
             << result.simulatedBer << ","
             << result.theoreticalBer << ","
             << result.totalBits << ","
             << result.bitErrors
             << "\n";
    }

    // File is automatically closed when the function ends.
}