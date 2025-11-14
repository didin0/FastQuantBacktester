#pragma once

#include "Strategy.h"
#include "../DataLoader/Candle.h"
#include <deque>
#include <vector>
#include <chrono>
#include <string>

namespace fastquant {

enum class SignalType { Buy, Sell };

struct Signal {
    std::chrono::system_clock::time_point timestamp;
    SignalType type;
    double price;
};

class MovingAverageStrategy : public Strategy {
public:
    MovingAverageStrategy(size_t shortWindow = 5, size_t longWindow = 20);

    void onStart() override;
    void onData(const Candle& candle) override;
    void onFinish() override;

    const std::vector<Signal>& signals() const { return signals_; }

private:
    size_t shortWindow_;
    size_t longWindow_;
    std::deque<double> shortBuf_;
    std::deque<double> longBuf_;
    double shortSum_ = 0.0;
    double longSum_ = 0.0;
    // whether short MA was above long MA in previous step (to detect crossings)
    bool lastShortAboveLong_ = false;

    std::vector<Signal> signals_;
    // default order quantity for emitted orders
    double orderQty_ = 1.0;
    std::string lastSymbol_ = "";
};

} // namespace fastquant
