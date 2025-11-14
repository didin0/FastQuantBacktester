#pragma once

#include <string>
#include <chrono>

namespace fastquant {

enum class Side { Buy, Sell };

struct Order {
    std::string id; // optional client id
    Side side = Side::Buy;
    double price = 0.0; // limit price or execution hint
    double qty = 0.0;
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
};

// Sink interface strategies use to submit orders to the engine.
struct OrderSink {
    virtual ~OrderSink() = default;
    virtual void submit(const Order& order) = 0;
};

} // namespace fastquant
