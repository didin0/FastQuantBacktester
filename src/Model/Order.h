#pragma once

#include <string>
#include <chrono>

namespace fastquant {

enum class Side { Buy, Sell };

enum class OrderType { Market, Limit };

enum class TimeInForce { GTC, IOC, FOK };

struct Order {
    std::string id; // optional client id
    Side side = Side::Buy;
    OrderType type = OrderType::Market;
    TimeInForce tif = TimeInForce::GTC;
    double price = 0.0; // limit price or execution hint
    double qty = 0.0;
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
    double slippageBps = 0.0; // optional slippage override per order
};

// Sink interface strategies use to submit orders to the engine.
struct OrderSink {
    virtual ~OrderSink() = default;
    virtual void submit(const Order& order) = 0;
};

} // namespace fastquant
