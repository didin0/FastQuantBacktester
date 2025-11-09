#pragma once

#include <chrono>
#include <string>

namespace fastquant {

struct Candle {
    std::chrono::system_clock::time_point timestamp;
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    std::string symbol; // optional
};

} // namespace fastquant
