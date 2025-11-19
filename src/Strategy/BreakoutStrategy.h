#pragma once

#include "Strategy.h"
#include <chrono>
#include <deque>
#include <string>
#include <vector>

namespace fastquant {

enum class BreakoutSignalType { Buy, Sell };

struct BreakoutSignal {
    std::chrono::system_clock::time_point timestamp;
    BreakoutSignalType type;
    double price;
};

class BreakoutStrategy : public Strategy {
public:
    BreakoutStrategy(size_t lookback = 20,
                     double buffer = 0.0,
                     double orderQty = 1.0,
                     bool allowShort = false);

    void onStart() override;
    void onData(const Candle& candle) override;
    void onFinish() override {}

    const std::vector<BreakoutSignal>& signals() const { return signals_; }

private:
    enum class PositionState { Flat, Long, Short };

    void submitOrder(Side side, double price, const Candle& candle);
    void recordSignal(BreakoutSignalType type, double price, const Candle& candle);

    size_t lookback_;
    double buffer_;
    double orderQty_;
    bool allowShort_;

    std::deque<double> highs_;
    std::deque<double> lows_;

    PositionState position_{PositionState::Flat};
    std::string lastSymbol_;
    std::vector<BreakoutSignal> signals_;
    size_t orderCounter_{0};
};

} // namespace fastquant
