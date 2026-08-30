#pragma once
#include "SecdedEncoder.hpp"
#include <vector>
#include <cstdint>
#include <stdexcept>

enum class SecdedStatus
{
    NoError,
    SingleCorrected,
    DoubleDetectedUncorrectable
};

struct SecdedDecodeResult
{
    std::vector<uint8_t> infoBits;   // recovered 11 info bits (best effort;
                                      // meaningless if status == DoubleDetectedUncorrectable)
    SecdedStatus status;
};

class SecdedDecoder
{
public:
    // received.size() must be exactly 16.
    SecdedDecodeResult decode(const std::vector<uint8_t>& received) const
    {
        if (received.size() != HammingEncoder::kCodedBits)
            throw std::invalid_argument("SecdedDecoder::decode: expected 16 coded bits");

        std::vector<uint8_t> cw = received; // mutable local copy, corrected in place

        // Syndrome: for each Hamming parity position p, XOR of every
        // position i in [1..15] whose bit-pattern includes p (this
        // naturally includes the parity bit itself).
        int syndrome = 0;
        for (int p : HammingEncoder::parityPositions)
        {
            uint8_t x = 0;
            for (int i = 1; i <= 15; ++i)
                if (i & p) x ^= cw[i];
            if (x != 0) syndrome |= p;
        }

        uint8_t overallParity = 0;
        for (int i = 0; i <= 15; ++i) overallParity ^= cw[i];

        SecdedStatus status;
        if (syndrome == 0 && overallParity == 0)
        {
            status = SecdedStatus::NoError;
        }
        else if (syndrome != 0 && overallParity != 0)
        {
            // Single error within positions 1..15 -- flip it.
            cw[syndrome] ^= 1;
            status = SecdedStatus::SingleCorrected;
        }
        else if (syndrome == 0 && overallParity != 0)
        {
            // Error is isolated to the overall-parity bit itself (position 0);
            // data bits are unaffected.
            cw[0] ^= 1;
            status = SecdedStatus::SingleCorrected;
        }
        else // syndrome != 0 && overallParity == 0
        {
            status = SecdedStatus::DoubleDetectedUncorrectable;
        }

        std::vector<uint8_t> infoBits(HammingEncoder::kInfoBits);
        for (std::size_t k = 0; k < HammingEncoder::kInfoBits; ++k)
            infoBits[k] = cw[HammingEncoder::dataPositions[k]];

        return {infoBits, status};
    }
};