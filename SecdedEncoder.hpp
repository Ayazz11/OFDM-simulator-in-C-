#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <array>

class HammingEncoder
{
public:
    static const std::size_t kInfoBits = 11;
    static const std::size_t kCodedBits = 16;

    // Data-bit positions within the 16-bit codeword (position 0 = overall
    // parity, positions 1,2,4,8 = Hamming parity -- everything else is data).
    static constexpr std::array<int, kInfoBits> dataPositions = {3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15};
    static constexpr std::array<int, 4> parityPositions = {1, 2, 4, 8};

    // infoBits.size() must be exactly 11. Returns a 16-bit codeword.
    std::vector<uint8_t> encode(const std::vector<uint8_t>& infoBits) const
    {
        if (infoBits.size() != kInfoBits){
            throw std::invalid_argument("HammingEncoder::encode: expected 11 info bits");}
        // cw is the codeword of size 16 for the 11 bit infoBits.
        std::vector<uint8_t> cw(kCodedBits, 0);
        for (std::size_t k = 0; k < kInfoBits; ++k){
            cw[dataPositions[k]] = infoBits[k];
        }
        // Each Hamming parity bit is the XOR of all positions it covers
        // (itself excluded from the sum, then set so the total covered set
        // XORs to zero).
        for (int p : parityPositions)
        {
            uint8_t x = 0;
            for (int i = 1; i <= 15; ++i)
            {
                if(i!=p){
                    if(i&p){
                        x=x^cw[i];
                    }
                }
            }
            cw[p] = x;
        }

        // Overall parity over positions 1..15 (even parity across the whole
        // 16-bit word once cw[0] is set).
        uint8_t overall = 0;
        for (int i = 1; i <= 15; ++i) {overall ^= cw[i];}
        cw[0] = overall;
        return cw;
    }
};
