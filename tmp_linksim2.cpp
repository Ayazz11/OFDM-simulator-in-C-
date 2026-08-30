#include "LinkSimulator2.hpp"
#include <iostream>

int main() {
    SystemConfig cfg;
    cfg.bitsPerSymbol = 4;
    cfg.numUsedSubcarriers = 52;
    cfg.numTrialsPerSnr = 5;
    cfg.numOfdmSymbolsPerTrial = 5;
    cfg.snrStartDb = 0.0;
    cfg.snrStopDb = 4.0;
    cfg.snrStepDb = 2.0;
    LinkSimulator2 sim(cfg);
    auto results = sim.runBerSweep();
    std::cout << "points=" << results.size() << "\n";
    return 0;
}
