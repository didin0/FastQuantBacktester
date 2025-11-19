#include "BreakoutStrategy.h"

#include <algorithm>
#include <limits>

namespace fastquant {

BreakoutStrategy::BreakoutStrategy(size_t lookback, double buffer, double orderQty, bool allowShort)
        : lookback_(lookback ? lookback : 1),
            buffer_(buffer),
            orderQty_(orderQty > 0 ? orderQty : 1.0),
            allowShort_(allowShort) {}

void BreakoutStrategy::onStart() {
    highs_.clear();
    lows_.clear();
    position_ = PositionState::Flat;
    signals_.clear();
    orderCounter_ = 0;
}

void BreakoutStrategy::onData(const Candle& candle) {
    if (!candle.symbol.empty()) {
        lastSymbol_ = candle.symbol;
    } else if (lastSymbol_.empty()) {
        lastSymbol_ = "DEFAULT";
    }

    if (highs_.size() < lookback_ || lows_.size() < lookback_) {
        // Need historical context first; store candle and wait.
        highs_.push_back(candle.high);
        lows_.push_back(candle.low);
        return;
    }

    const double highestHigh = *std::max_element(highs_.begin(), highs_.end());
    const double lowestLow = *std::min_element(lows_.begin(), lows_.end());
    const double price = candle.close;

    bool handled = false;
    if (!handled && position_ == PositionState::Long && price < (lowestLow - buffer_)) {
        submitOrder(Side::Sell, price, candle);
        recordSignal(BreakoutSignalType::Sell, price, candle);
        position_ = PositionState::Flat;
        handled = true;
    }

    if (!handled && position_ == PositionState::Short && price > (highestHigh + buffer_)) {
        submitOrder(Side::Buy, price, candle);
        recordSignal(BreakoutSignalType::Buy, price, candle);
        position_ = PositionState::Flat;
        handled = true;
    }

    if (!handled && price > (highestHigh + buffer_) && position_ == PositionState::Flat) {
        submitOrder(Side::Buy, price, candle);
        recordSignal(BreakoutSignalType::Buy, price, candle);
        position_ = PositionState::Long;
        handled = true;
    }

    if (!handled && allowShort_ && price < (lowestLow - buffer_) && position_ == PositionState::Flat) {
        submitOrder(Side::Sell, price, candle);
        recordSignal(BreakoutSignalType::Sell, price, candle);
        position_ = PositionState::Short;
    }

    highs_.push_back(candle.high);
    lows_.push_back(candle.low);
    if (highs_.size() > lookback_) {
        highs_.pop_front();
    }
    if (lows_.size() > lookback_) {
        lows_.pop_front();
    }
}

void BreakoutStrategy::submitOrder(Side side, double price, const Candle& candle) {
    if (!orderSink_) {
        return;
    }

    Order order;
    order.id = "breakout-" + std::to_string(++orderCounter_);
    order.symbol = lastSymbol_;
    order.timestamp = candle.timestamp;
    order.side = side;
    order.qty = orderQty_;
    order.type = OrderType::Market;
    order.price = price;

    orderSink_->submit(order);
}

void BreakoutStrategy::recordSignal(BreakoutSignalType type, double price, const Candle& candle) {
    signals_.push_back({candle.timestamp, type, price});
}

} // namespace fastquant
