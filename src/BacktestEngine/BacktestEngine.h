#pragma once

#include <string>
#include <memory>
#include "../DataLoader/CSVDataLoader.h"
#include "../Strategy/Strategy.h"
#include "../Model/Trade.h"
#include "../Model/Order.h"
#include "../Model/Portfolio.h"
#include <vector>
#include <chrono>
#include <functional>

namespace fastquant {

struct ExecutionConfig {
    double defaultSlippageBps{0.0};
    double commissionPerShare{0.0};
    double commissionBps{0.0};
};

struct BacktestResult {
    size_t candlesProcessed{0};
    std::vector<Trade> trades;
    Portfolio portfolio;
    std::vector<double> equityCurve;
    std::vector<std::chrono::system_clock::time_point> equityTimestamps;
    double initialCapital{0.0};
    double totalFees{0.0};
    double totalSlippage{0.0};
    size_t ordersFilled{0};
    size_t ordersRejected{0};

    BacktestResult();
    explicit BacktestResult(double initialCapital);
};

class BacktestEngine {
public:
    explicit BacktestEngine(ExecutionConfig exec = ExecutionConfig{});

    void setExecutionConfig(const ExecutionConfig& cfg) { execConfig_ = cfg; }
    const ExecutionConfig& executionConfig() const { return execConfig_; }

    // Run backtest by streaming CSV into the provided strategy.
    BacktestResult run(const std::string& csvPath,
                       CSVDataLoader::Config cfg,
                       Strategy& strategy,
                       double initialCapital = 100000.0);

    BacktestResult run(const std::vector<Candle>& candles,
                       Strategy& strategy,
                       double initialCapital = 100000.0);

    std::vector<BacktestResult> runParallel(const std::string& csvPath,
                                            CSVDataLoader::Config cfg,
                                            const std::vector<std::function<std::unique_ptr<Strategy>()>>& strategyFactories,
                                            double initialCapital = 100000.0);

    std::vector<BacktestResult> runParallel(const std::vector<Candle>& candles,
                                            const std::vector<std::function<std::unique_ptr<Strategy>()>>& strategyFactories,
                                            double initialCapital = 100000.0);

private:
    ExecutionConfig execConfig_;
    BacktestResult runWithStreamer(const std::function<void(const std::function<bool(const Candle&)>&)>& streamer,
                                   Strategy& strategy,
                                   double initialCapital);
};

} // namespace fastquant
