#pragma once

#include <string>
#include <chrono>
#include "Order.h"

namespace fastquant {

struct Trade {
    std::string id;
    std::string orderId;
    Side side = Side::Buy;
    OrderType type = OrderType::Market;
    double price = 0.0;
    double qty = 0.0;
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
    double fee{0.0};
    double slippageBps{0.0};
};

} // namespace fastquant
