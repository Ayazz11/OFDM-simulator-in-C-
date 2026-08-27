#pragma once
#include <vector>
#include <cstdint>
#include <random>
class BitSource{
public:
    // constructor is used to assign the generator a seed, everytime the same bit sequence will be generated, helps in comparision of different modulation scemes and channel
    BitSource(unsigned int seed)
    {
        generator.seed(seed);
    }
    //generate bits function :
    std::vector<uint8_t> generate(std::size_t numberOfBits)
    {
        std::vector<uint8_t> bits(numberOfBits);
        for (std::size_t i = 0; i < numberOfBits; i++)
        {
            bits[i] = distribution(generator);
        }
        return bits;
    }

private:
    std::mt19937 generator;
    std::uniform_int_distribution<int> distribution{0, 1};
};