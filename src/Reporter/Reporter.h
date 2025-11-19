#pragma once

#include "../BacktestEngine/BacktestEngine.h"
#include <string>

namespace fastquant {

struct ReportSummary {
    double initialCapital{0.0};
    double finalEquity{0.0};
    double totalReturn{0.0}; // (final / initial) - 1
    double realizedPnl{0.0};
    double unrealizedPnl{0.0};
    double maxDrawdown{0.0}; // expressed as positive value
    double peakEquity{0.0};
    double troughEquity{0.0};
    size_t trades{0};
    size_t winningTrades{0};
    size_t losingTrades{0};
    double winRate{0.0};
    double totalFees{0.0};
    double totalSlippage{0.0};
    size_t ordersFilled{0};
    size_t ordersRejected{0};
};

class Reporter {
public:
    ReportSummary summarize(const BacktestResult& result) const;

    void writeJson(const BacktestResult& result, const std::string& path) const;
    void writeSummaryCsv(const BacktestResult& result, const std::string& path) const;
    void writeTradesCsv(const BacktestResult& result, const std::string& path) const;

private:
    static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);
};

} // namespace fastquant
