#pragma once

#include "SystemConfig.hpp"
#include "BitSource.hpp"
#include "QamModulator.hpp"
#include "OfdmModulator.hpp"
#include "Channel.hpp"
#include "BerAnalyzer.hpp"
#include "SecdedEncoder.hpp"
#include "SecdedDecoder.hpp"
#include "LinkSimulator.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class LinkSimulator2
{
public:
    explicit LinkSimulator2(const SystemConfig& cfg) : config(cfg) {}

    // Optional sanity test: one coded OFDM pass should round-trip without channel loss.
    bool sanityCheckLoopback() const
    {
        BitSource bitSource(config.randomSeed);
        QamModulator qam(config.bitsPerSymbol);
        OfdmModulator ofdm(config);
        HammingEncoder encoder;
        SecdedDecoder decoder;

        const std::size_t bitsPerOfdmSymbol = config.numUsedSubcarriers * config.bitsPerSymbol;
        const std::size_t codewordsPerFrame = bitsPerOfdmSymbol / HammingEncoder::kCodedBits;
        const std::size_t infoBitsPerFrame = codewordsPerFrame * HammingEncoder::kInfoBits;

        std::vector<uint8_t> transmittedInfoBits = bitSource.generate(infoBitsPerFrame * config.numOfdmSymbolsPerTrial);
        std::vector<uint8_t> receivedInfoBits;
        receivedInfoBits.reserve(transmittedInfoBits.size());

        for (std::size_t symbol = 0; symbol < config.numOfdmSymbolsPerTrial; ++symbol)
        {
            std::vector<uint8_t> frameInfoBits(
                transmittedInfoBits.begin() + symbol * infoBitsPerFrame,
                transmittedInfoBits.begin() + (symbol + 1) * infoBitsPerFrame);

            std::vector<uint8_t> codedBits;
            codedBits.reserve(codewordsPerFrame * HammingEncoder::kCodedBits);
            for (std::size_t c = 0; c < codewordsPerFrame; ++c)
            {
                std::vector<uint8_t> chunk(
                    frameInfoBits.begin() + c * HammingEncoder::kInfoBits,
                    frameInfoBits.begin() + (c + 1) * HammingEncoder::kInfoBits);
                auto codedChunk = encoder.encode(chunk);
                codedBits.insert(codedBits.end(), codedChunk.begin(), codedChunk.end());
            }

            std::vector<Complex> qamSymbols = qam.modulate(codedBits);
            std::vector<Complex> txWaveform = ofdm.modulate(qamSymbols);
            std::vector<Complex> rxWaveform = ofdm.demodulate(txWaveform);
            std::vector<uint8_t> decodedCodedBits = qam.demodulate(rxWaveform);

            std::vector<uint8_t> recoveredInfoBits;
            for (std::size_t i = 0; i < decodedCodedBits.size(); i += HammingEncoder::kCodedBits)
            {
                std::vector<uint8_t> codeword(
                    decodedCodedBits.begin() + i,
                    decodedCodedBits.begin() + i + HammingEncoder::kCodedBits);
                if (codeword.size() != HammingEncoder::kCodedBits)
                    break;
                auto decoded = decoder.decode(codeword);
                recoveredInfoBits.insert(recoveredInfoBits.end(), decoded.infoBits.begin(), decoded.infoBits.end());
            }

            receivedInfoBits.insert(
                receivedInfoBits.end(),
                recoveredInfoBits.begin(),
                recoveredInfoBits.end());
        }

        return transmittedInfoBits == receivedInfoBits;
    }

    std::vector<BerResultPoint> runBerSweep() const
    {
        BitSource bitSource(config.randomSeed);
        QamModulator qam(config.bitsPerSymbol);
        OfdmModulator ofdm(config);
        Channel channel_awgn(config.randomSeed + 1);
        HammingEncoder encoder;
        SecdedDecoder decoder;

        const std::size_t bitsPerOfdmSymbol = config.numUsedSubcarriers * config.bitsPerSymbol;
        const std::size_t codewordsPerFrame = bitsPerOfdmSymbol / HammingEncoder::kCodedBits;
        const std::size_t infoBitsPerFrame = codewordsPerFrame * HammingEncoder::kInfoBits;

        std::vector<BerResultPoint> results;

        for (double snrDb = config.snrStartDb; snrDb <= config.snrStopDb; snrDb += config.snrStepDb)
        {
            std::size_t totalBits = 0;
            std::size_t totalErrors = 0;
            std::size_t singleBitCorrected = 0;
            std::size_t doubleBitDetected = 0;

            for (std::size_t trial = 0; trial < config.numTrialsPerSnr; ++trial)
            {
                std::vector<uint8_t> totalInfoBits;
                totalInfoBits.reserve(infoBitsPerFrame * config.numOfdmSymbolsPerTrial);

                for (std::size_t symbol = 0; symbol < config.numOfdmSymbolsPerTrial; ++symbol)
                {
                    std::vector<uint8_t> infoBits = bitSource.generate(infoBitsPerFrame);
                    std::vector<uint8_t> codedBits;
                    codedBits.reserve(codewordsPerFrame * HammingEncoder::kCodedBits);

                    for (std::size_t c = 0; c < codewordsPerFrame; ++c)
                    {
                        std::vector<uint8_t> chunk(
                            infoBits.begin() + c * HammingEncoder::kInfoBits,
                            infoBits.begin() + (c + 1) * HammingEncoder::kInfoBits);
                        auto encodedChunk = encoder.encode(chunk);
                        codedBits.insert(codedBits.end(), encodedChunk.begin(), encodedChunk.end());
                    }

                    std::vector<Complex> qamSymbols = qam.modulate(codedBits);
                    std::vector<Complex> txWaveform = ofdm.modulate(qamSymbols);
                    std::vector<Complex> rxWaveform = channel_awgn.addAwgn(txWaveform, snrDb, config.bitsPerSymbol);
                    std::vector<Complex> rxQamSymbols = ofdm.demodulate(rxWaveform);
                    std::vector<uint8_t> recoveredCodedBits = qam.demodulate(rxQamSymbols);

                    std::vector<uint8_t> recoveredInfoBits;
                    recoveredInfoBits.reserve(infoBits.size());
                    for (std::size_t i = 0; i < recoveredCodedBits.size(); i += HammingEncoder::kCodedBits)
                    {
                        std::vector<uint8_t> codeword(
                            recoveredCodedBits.begin() + i,
                            recoveredCodedBits.begin() + i + HammingEncoder::kCodedBits);
                        if (codeword.size() != HammingEncoder::kCodedBits)
                            break;

                        auto result = decoder.decode(codeword);
                        if (result.status == SecdedStatus::SingleCorrected)
                            ++singleBitCorrected;
                        if (result.status == SecdedStatus::DoubleDetectedUncorrectable)
                            ++doubleBitDetected;

                        recoveredInfoBits.insert(
                            recoveredInfoBits.end(),
                            result.infoBits.begin(),
                            result.infoBits.end());
                    }

                    for (std::size_t i = 0; i < infoBits.size() && i < recoveredInfoBits.size(); ++i)
                    {
                        if (infoBits[i] != recoveredInfoBits[i])
                            ++totalErrors;
                    }

                    totalBits += infoBits.size();
                    totalInfoBits.insert(totalInfoBits.end(), infoBits.begin(), infoBits.end());
                }
            }

            double simulatedBer = static_cast<double>(totalErrors) / static_cast<double>(totalBits);
            double theoreticalBer = BerAnalyzer::theoreticalBerAWGN(snrDb, config.bitsPerSymbol);

            BerResultPoint result;
            result.snrDb = snrDb;
            result.simulatedBer = simulatedBer;
            result.theoreticalBer = theoreticalBer;
            result.totalBitsSimulated = totalBits;
            result.totalBitErrors = totalErrors;
            results.push_back(result);

            std::cout << "SNR = " << snrDb << " dB"
                      << " | Simulated BER = " << simulatedBer
                      << " | Theoretical BER = " << theoreticalBer
                      << " | Errors = " << totalErrors
                      << " / " << totalBits
                      << " | 1-bit corrected = " << singleBitCorrected
                      << " | 2-bit detected = " << doubleBitDetected
                      << std::endl;
        }

        return results;
    }

    static void writeCsv(const std::string& filePath, const std::vector<BerResultPoint>& results)
    {
        std::ofstream file(filePath);
        if (!file)
        {
            throw std::runtime_error("Could not open file: " + filePath);
        }

        file << "snr_db,"
             << "simulated_ber,"
             << "theoretical_ber,"
             << "total_bits,"
             << "bit_errors\n";

        for (const BerResultPoint& result : results)
        {
            file << result.snrDb << ","
                 << result.simulatedBer << ","
                 << result.theoreticalBer << ","
                 << result.totalBitsSimulated << ","
                 << result.totalBitErrors << "\n";
        }
    }

private:
    SystemConfig config;
};
