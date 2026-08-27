#pragma once
#include <vector>
#include <complex>
#include <cstdint>
#include <cmath>
#include <stdexcept>
using Complex = std::complex<float>;

class QamModulator{
public:
    // constructor : user will initialize the QamModulator with the number of bits per symbol (2, 4, or 6). The constructor checks if the provided bitsPerSymbol is valid and calculates the necessary parameters for modulation and demodulation.
    QamModulator(std::size_t bps)
    {
        if (bps != 2 && bps != 4 && bps != 6)
        {
            throw std::invalid_argument("Only QPSK, 16-QAM and 64-QAM are supported.");
        }
        //Initializing the class data members
        this->bitsPerSymbol = bps;
        //bits dedicated for I and Q components. 
        this->bitsPerDimension = bitsPerSymbol / 2;
        // Number of amplitude levels along I or Q.
        this->levelsPerDimension = 1 << bitsPerDimension; // 1<<n is equivalent to 2^n.
        // Normalize average symbol energy to 1. we assume the bit energy is 1. The normalization factor is calculated based on the number of levels per dimension. This is to make sure that the average energy of the modulated symbols is 1 to simplify analysis and comparison of different modulation schemes. 
        calculateNormalizationFactor(levelsPerDimension);
    }

 // simple accessor to retrieve the number of bits per symbol.
    std::size_t getBitsPerSymbol() const
    {
        return bitsPerSymbol;
    }

//Modulation : maps bits to symbols , eg : 1011 ---> complex number
    std::vector<Complex> modulate(const std::vector<uint8_t>& bits)
    {
        // Number of input bits must be divisible by bitsPerSymbol.
        if (bits.size() % bitsPerSymbol != 0)
        {
            throw std::invalid_argument("Number of bits must be a multiple of bitsPerSymbol.");
        }
        std::size_t numberOfSymbols = bits.size() / bitsPerSymbol;
        std::vector<Complex> symbols(numberOfSymbols);
        // Process one QAM symbol at a time.
        for (std::size_t symbolIndex = 0; symbolIndex < numberOfSymbols; symbolIndex++)
        {
            // Determine Starting position of this symbol's bits. eg : for 16-QAM, bitsPerSymbol = 4, so for symbolIndex = 0, start = 0; for symbolIndex = 1, start = 4; for symbolIndex = 2, start = 8; and so on.
            std::size_t start = symbolIndex * bitsPerSymbol;
            // The first half of the bits are for the I component, and the second half are for the Q component. For example, in 16-QAM, if the bits for a symbol are 1011, then 10 is for I and 11 is for Q.
            //ibits and qbits are creates again for every iteration, they only store half of the bits per symbol.
            std::vector<uint8_t> iBits( bits.begin() + start, bits.begin() + start + bitsPerDimension);
            std::vector<uint8_t> qBits( bits.begin() + start + bitsPerDimension, bits.begin() + start + bitsPerSymbol);

            // Convert bits to amplitudes.
            float I = bitsToAmplitude(iBits);
            float Q = bitsToAmplitude(qBits);
            // Create complex QAM symbol.
            symbols[symbolIndex] = Complex(I * normalizationFactor, Q * normalizationFactor);
        }
        return symbols;
    }

//De-Modulation : maps symbols to bits , eg : Complex Number ---> 1011
    std::vector<uint8_t> demodulate(const std::vector<Complex>& symbols)
    {
        std::vector<uint8_t> bits;
        // Reserving enough memory beforehand. This does not change the size of the vector, but it avoids multiple reallocations as we append bits.
        bits.reserve(symbols.size() * bitsPerSymbol);
        for (const Complex& symbol : symbols)
        {
            // Remove normalization.
            float I = symbol.real() / normalizationFactor;
            float Q = symbol.imag() / normalizationFactor;

            // Convert I and Q back into bits.
            std::vector<uint8_t> iBits = amplitudeToBits(I);
            std::vector<uint8_t> qBits = amplitudeToBits(Q);

            // Add them to the output bitstream.
            bits.insert(bits.end(), iBits.begin(), iBits.end());
            bits.insert(bits.end(), qBits.begin(), qBits.end());
        }
        return bits;
    }


private:

    // bits to amplitude : eg 01 --> -1, 00--> -3.
    float bitsToAmplitude(const std::vector<uint8_t>& bits)
    {
       // The input bits (ibits or qbits) are already assumed to be Gray-coded.
       // Step 1:
       // Combine the individual bits from the vector into one Gray-code number representaion.
       // The binary value of the number is actually our input and assumed to be gray code.
        unsigned int grayNumber = 0;
        for (uint8_t bit : bits)
        {
            grayNumber = (grayNumber << 1) | bit;
        }
        //convert the gray coded number to binary .
        unsigned int binaryNumber = grayToBinary(grayNumber);
        // Amplitude mapping
        int amplitude = 2 * binaryNumber - (levelsPerDimension - 1);
        return static_cast<float>(amplitude);
    }

    std::vector<uint8_t> amplitudeToBits(float amplitude)
    {
        // find nearest PAM level
        int maximumIndex = levelsPerDimension - 1;
        int binaryNumber = static_cast<int>(std::round((amplitude + maximumIndex) / 2.0f));

        // Make sure index stays inside valid range.
        if (binaryNumber < 0){
            binaryNumber = 0;
        }
        if (binaryNumber > maximumIndex){
            binaryNumber = maximumIndex;
        }
        // Step 2:
        // Binary number -> Gray code.
        unsigned int grayNumber = binaryToGray(binaryNumber);

        // Gray code -> individual bits.
        std::vector<uint8_t> bits(bitsPerDimension);
        for (std::size_t i = 0; i < bitsPerDimension; i++)
        {
            int shift = bitsPerDimension - 1 - i;
            bits[i] = (grayNumber >> shift) & 1;
        }
        return bits;
    }   

//Useful functions
//Binary to gray = (b)XOR(b>>1)
    unsigned int binaryToGray(unsigned int binary)
    {
        return binary ^ (binary >> 1);
    }
//Gray to binary. 
    unsigned int grayToBinary(unsigned int gray)
    {
        unsigned int binary = 0;
        while (gray > 0)
        {
            binary = binary^gray;
            gray = gray>>1;
        }
        return binary;
    }
// Calculate normalization factor.
    void calculateNormalizationFactor(size_t L)
    {
        float averageEnergy = 2.0f * (L * L - 1.0f) / 3.0f;
        normalizationFactor =1.0f / std::sqrt(averageEnergy);
    }

private:
    std::size_t bitsPerSymbol;
    std::size_t bitsPerDimension;
    std::size_t levelsPerDimension;
    float normalizationFactor;
};