#include <catch2/catch.hpp>
#include "../src/Strategy/MovingAverageStrategy.h"
#include "../src/DataLoader/Candle.h"

using namespace fastquant;

static Candle make_candle(double price, std::time_t t = 0) {
    Candle c;
    c.close = price;
    c.open = price;
    c.high = price;
    c.low = price;
    c.volume = 1.0;
    c.timestamp = std::chrono::system_clock::from_time_t(t);
    return c;
}

TEST_CASE("MovingAverageStrategy generates buy and sell signals", "ma") {
    MovingAverageStrategy strat(3, 5);
    strat.onStart();

    // Construct a price sequence: flat, then rising (should buy), then falling (should sell)
    std::vector<double> prices = {100,100,100,100,100, 101,102,103,104,105, 104,103,102,101,100};

    std::time_t t = 0;
    for (double p : prices) {
        strat.onData(make_candle(p, t++));
    }

    strat.onFinish();

    const auto& sigs = strat.signals();
    REQUIRE(sigs.size() >= 2);
    REQUIRE(sigs.front().type == SignalType::Buy);
    REQUIRE(sigs.back().type == SignalType::Sell);
}
