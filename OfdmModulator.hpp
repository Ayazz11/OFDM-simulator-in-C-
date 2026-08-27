#pragma once
#include "SystemConfig.hpp"
#include "kissfft.hh"
#include <vector>
#include <complex>
#include <stdexcept>
#include <cmath>
#include <algorithm>
using Complex = std::complex<float>;

class OfdmModulator{
public:
    // Constructor
    // Takes the system configuration and sets the config for each ofdm modulator, initialize  the FFT/IFFT size.
         OfdmModulator(const SystemConfig& sysconfig)
        : config(sysconfig),
          fft(sysconfig.fftSize, false),
          ifft(sysconfig.fftSize, true)
    {
        // Validate the configuration
        validateConfiguration();
        // This is for the FFT bins to qam symbols mapping.
        createSubcarrierIndices();
    }

// OFDM Modulation : input will be the vector containing complex qamSymbols, output will be the time domain digital samples including cyclic prefix.
    std::vector<Complex> modulate(const std::vector<Complex>& qamSymbols)
    {
        // We should receive one QAM symbol
        // for every active subcarrier.
        if (qamSymbols.size() != config.numUsedSubcarriers)
        {
            throw std::invalid_argument(
                "Wrong number of QAM symbols."
            );
        }
        // Step 1: Create the frequency-domain OFDM grid.
        // frequencyGrid[] = [0, S0, S1, ..., S25, 0, ..., 0, S26, ..., S51]
        std::vector<Complex> frequencyGrid(config.fftSize, Complex(0.0f, 0.0f));
        // Put each QAM symbol into its correct subcarrier.
        for (std::size_t i = 0;i < qamSymbols.size(); i++)
        {
            frequencyGrid[subcarrierIndices[i]] =qamSymbols[i];
        }

        // Step 2: Perform IFFT.
        std::vector<Complex> timeSignal(config.fftSize); // this will store the output time domain samples.
        ifft.transform(frequencyGrid.data(),timeSignal.data()); // ifft.transform(i,o) where i and o are pointers to the linear data
        for (auto& sample : timeSignal)
        {
            sample /= static_cast<float>(sqrt(config.fftSize));
        }
        //Add CP
        return addCyclicPrefix(timeSignal);
    }

    //OFDM DEMODULATION
    std::vector<Complex> demodulate(const std::vector<Complex>& receivedSignal) {

        std::size_t expectedSize = config.fftSize + config.cpLen;
        if (receivedSignal.size() != expectedSize)
        {
            throw std::invalid_argument(
                "Wrong received OFDM signal length."
            );
        }
        //Remove CP
        std::vector<Complex> signalWithoutCP =removeCyclicPrefix(receivedSignal);
        //FFT
        std::vector<Complex> frequencyGrid(config.fftSize);
        for (auto& sample : signalWithoutCP)
        {
            sample /= static_cast<float>(sqrt(config.fftSize));
        }
        fft.transform(signalWithoutCP.data(),frequencyGrid.data());
        // Extract the active subcarriers
        std::vector<Complex> qamSymbols(config.numUsedSubcarriers);
        for (std::size_t i = 0;i < config.numUsedSubcarriers;i++)
        {
            qamSymbols[i] = frequencyGrid[subcarrierIndices[i]];
        }
        return qamSymbols;
    }

    // Accessor : Allows other parts of the program to seE which FFT bins are being used.
    const std::vector<std::size_t>& getSubcarrierIndices() const
    {
        return subcarrierIndices;
    }


private:
    // System configuration
    SystemConfig config;
    // FFT and IFFT objects
    kissfft<float> fft;
    kissfft<float> ifft;
    std::vector<std::size_t> subcarrierIndices;  // Stores the FFT bin numbers that carry data.

    void validateConfiguration(){
        if (config.numUsedSubcarriers % 2 != 0)
        {
            throw std::invalid_argument("Number of used subcarriers must be even.");
        }
        // Make sure there are enough FFT bins.
        if (config.numUsedSubcarriers >= config.fftSize)
        {
            throw std::invalid_argument("Too many used subcarriers for the FFT size."            );
        }
    }

    void createSubcarrierIndices()
    {
        std::size_t half =config.numUsedSubcarriers / 2;
        subcarrierIndices.resize(config.numUsedSubcarriers);
        for (std::size_t i = 0;i < half;i++)
        {
            subcarrierIndices[i] =1 + i;
            subcarrierIndices[half + i] = config.fftSize - half + i;
        }
    }

    std::vector<Complex> addCyclicPrefix(const std::vector<Complex>& timeSignal)
    {
        std::vector<Complex> signalWithCP;
        // Reserve the required memory.
        signalWithCP.reserve(config.fftSize + config.cpLen);
        //copy the last CP samples first
        std::size_t cpStart =timeSignal.size() - config.cpLen;
        for (std::size_t i = cpStart;i < timeSignal.size();i++)
        {
        signalWithCP.push_back(timeSignal[i]);
        }
        // Then copy the complete OFDM symbol.
        for (const Complex& sample : timeSignal)
        {
        signalWithCP.push_back(sample);
        }
        return signalWithCP;
    }

    std::vector<Complex> removeCyclicPrefix(const std::vector<Complex>& receivedSignal)
    {
        std::vector<Complex> signalWithoutCP;
        signalWithoutCP.reserve(config.fftSize);
        // Start copying after the CP.
        for (std::size_t i = config.cpLen;i < receivedSignal.size();i++)
        {
        signalWithoutCP.push_back(receivedSignal[i]);
        }
        return signalWithoutCP;
    }
};