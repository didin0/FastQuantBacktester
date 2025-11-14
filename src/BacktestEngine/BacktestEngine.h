#pragma once

#include <string>
#include <memory>
#include "../DataLoader/CSVDataLoader.h"
#include "../Strategy/Strategy.h"
#include "../Model/Trade.h"
#include "../Model/Order.h"
#include "../Model/Portfolio.h"
#include <vector>

namespace fastquant {

struct BacktestResult {
    size_t candlesProcessed{0};
    std::vector<Trade> trades;
    Portfolio portfolio;

    BacktestResult();
    explicit BacktestResult(double initialCapital);
};

class BacktestEngine {
public:
    BacktestEngine() = default;

    // Run backtest by streaming CSV into the provided strategy.
    BacktestResult run(const std::string& csvPath,
                       CSVDataLoader::Config cfg,
                       Strategy& strategy,
                       double initialCapital = 100000.0);
};

} // namespace fastquant
