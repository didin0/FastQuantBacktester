#pragma once

#include <memory>
#include <string>
#include "../DataLoader/Candle.h"
#include "../Model/Order.h"

namespace fastquant {

// Strategy: abstract base class for trading strategies.
// Implementations should be inexpensive in onData since they run in the backtest loop.
class Strategy {
public:
    virtual ~Strategy() = default;

    // Called once before backtest starts.
    virtual void onStart() {}

    // Called for every incoming candle. Implementations may emit orders via a callback
    // or update internal state.
    virtual void onData(const Candle& candle) = 0;

    // Called once after backtest ends.
    virtual void onFinish() {}

    // Optional: connect an OrderSink so the strategy can submit orders to the engine.
    void setOrderSink(OrderSink* sink) { orderSink_ = sink; }

protected:
    OrderSink* orderSink_ = nullptr;
};

} // namespace fastquant
