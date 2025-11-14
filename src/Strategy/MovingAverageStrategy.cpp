#include "MovingAverageStrategy.h"

namespace fastquant {

MovingAverageStrategy::MovingAverageStrategy(size_t shortWindow, size_t longWindow)
    : shortWindow_(shortWindow), longWindow_(longWindow) {
    if (shortWindow_ == 0) shortWindow_ = 1;
    if (longWindow_ == 0) longWindow_ = 1;
    if (shortWindow_ > longWindow_) std::swap(shortWindow_, longWindow_);
}

void MovingAverageStrategy::onStart() {
    shortBuf_.clear();
    longBuf_.clear();
    shortSum_ = 0.0;
    longSum_ = 0.0;
    lastShortAboveLong_ = false;
    signals_.clear();
    lastSymbol_.clear();
}

void MovingAverageStrategy::onData(const Candle& candle) {
    double price = candle.close;
    if (!candle.symbol.empty()) {
        lastSymbol_ = candle.symbol;
    } else if (lastSymbol_.empty()) {
        lastSymbol_ = "DEFAULT";
    }

    shortBuf_.push_back(price);
    shortSum_ += price;
    if (shortBuf_.size() > shortWindow_) {
        shortSum_ -= shortBuf_.front();
        shortBuf_.pop_front();
    }

    longBuf_.push_back(price);
    longSum_ += price;
    if (longBuf_.size() > longWindow_) {
        longSum_ -= longBuf_.front();
        longBuf_.pop_front();
    }

    if (shortBuf_.size() == shortWindow_ && longBuf_.size() == longWindow_) {
        double shortMA = shortSum_ / static_cast<double>(shortWindow_);
        double longMA = longSum_ / static_cast<double>(longWindow_);

        bool nowShortAbove = (shortMA > longMA);
        if (!lastShortAboveLong_ && nowShortAbove) {
            // buy signal
            signals_.push_back({candle.timestamp, SignalType::Buy, price});
            if (orderSink_) {
                Order o;
                o.id = std::to_string(reinterpret_cast<uintptr_t>(this)) + "-buy-" + std::to_string(signals_.size());
                o.side = Side::Buy;
                o.price = price;
                o.qty = orderQty_;
                o.symbol = lastSymbol_;
                o.timestamp = candle.timestamp;
                orderSink_->submit(o);
            }
        } else if (lastShortAboveLong_ && !nowShortAbove) {
            // sell signal
            signals_.push_back({candle.timestamp, SignalType::Sell, price});
            if (orderSink_) {
                Order o;
                o.id = std::to_string(reinterpret_cast<uintptr_t>(this)) + "-sell-" + std::to_string(signals_.size());
                o.side = Side::Sell;
                o.price = price;
                o.qty = orderQty_;
                o.symbol = lastSymbol_;
                o.timestamp = candle.timestamp;
                orderSink_->submit(o);
            }
        }
        lastShortAboveLong_ = nowShortAbove;
    }
}

void MovingAverageStrategy::onFinish() {
    // no-op for now
}

} // namespace fastquant
