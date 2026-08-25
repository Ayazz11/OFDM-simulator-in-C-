#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "QamModulator.hpp"
#include <random>

TEST_CASE("QPSK map/demap round-trip", "[qam]")
{
    QamModulator qam(2);
    std::mt19937 rng(1);
    std::uniform_int_distribution<int> d(0, 1);

    std::vector<uint8_t> bits(2000);
    for (auto& b : bits) b = static_cast<uint8_t>(d(rng));

    auto symbols = qam.map(bits);
    REQUIRE(symbols.size() == bits.size() / 2);

    auto rxBits = qam.demap(symbols);
    REQUIRE(rxBits == bits);
}

TEST_CASE("16-QAM map/demap round-trip", "[qam]")
{
    QamModulator qam(4);
    std::mt19937 rng(2);
    std::uniform_int_distribution<int> d(0, 1);

    std::vector<uint8_t> bits(4000);
    for (auto& b : bits) b = static_cast<uint8_t>(d(rng));

    auto symbols = qam.map(bits);
    auto rxBits = qam.demap(symbols);
    REQUIRE(rxBits == bits);
}

TEST_CASE("64-QAM map/demap round-trip", "[qam]")
{
    QamModulator qam(6);
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> d(0, 1);

    std::vector<uint8_t> bits(6000);
    for (auto& b : bits) b = static_cast<uint8_t>(d(rng));

    auto symbols = qam.map(bits);
    auto rxBits = qam.demap(symbols);
    REQUIRE(rxBits == bits);
}

TEST_CASE("QAM average symbol energy is normalized to 1", "[qam]")
{
    for (std::size_t bps : {2, 4, 6})
    {
        QamModulator qam(bps);
        std::mt19937 rng(7);
        std::uniform_int_distribution<int> d(0, 1);

        const std::size_t numSymbols = 20000;
        std::vector<uint8_t> bits(numSymbols * bps);
        for (auto& b : bits) b = static_cast<uint8_t>(d(rng));

        auto symbols = qam.map(bits);
        double energy = 0.0;
        for (auto& s : symbols) energy += std::norm(s);
        energy /= static_cast<double>(symbols.size());

        REQUIRE(energy == Catch::Approx(1.0).margin(0.05));
    }
}

TEST_CASE("Invalid bitsPerSymbol throws", "[qam]")
{
    REQUIRE_THROWS_AS(QamModulator(3), std::invalid_argument);
    REQUIRE_THROWS_AS(QamModulator(8), std::invalid_argument);
}
