#include <catch2/catch.hpp>

#include "Strategy/BreakoutStrategy.h"

using namespace fastquant;

namespace {

struct RecordingSink : public OrderSink {
    void submit(const Order& order) override {
        orders.push_back(order);
    }

    std::vector<Order> orders;
};

Candle candle(double price, double high, double low, int day) {
    Candle c;
    c.symbol = "TEST";
    c.close = price;
    c.high = high;
    c.low = low;
    c.open = price;
    c.timestamp = std::chrono::system_clock::time_point{std::chrono::hours{24 * day}};
    return c;
}

} // namespace

TEST_CASE("BreakoutStrategy generates buy signals on upside breakouts") {
    BreakoutStrategy strategy(/*lookback*/ 3, /*buffer*/ 0.0, /*qty*/ 1.0, /*allowShort*/ false);
    RecordingSink sink;
    strategy.setOrderSink(&sink);
    strategy.onStart();

    strategy.onData(candle(10, 10, 9, 0));
    strategy.onData(candle(10.5, 10.5, 9.5, 1));
    strategy.onData(candle(11, 11, 10, 2));
    strategy.onData(candle(11.5, 11.5, 10.5, 3));

    REQUIRE(sink.orders.size() == 1);
    REQUIRE(sink.orders.front().side == Side::Buy);
    REQUIRE(strategy.signals().size() == 1);
    REQUIRE(strategy.signals().front().type == BreakoutSignalType::Buy);
}

TEST_CASE("BreakoutStrategy exits on downside breakouts") {
    BreakoutStrategy strategy(/*lookback*/ 3, 0.0, 1.0, false);
    RecordingSink sink;
    strategy.setOrderSink(&sink);
    strategy.onStart();

    strategy.onData(candle(10, 10, 9, 0));
    strategy.onData(candle(10.5, 10.5, 9.5, 1));
    strategy.onData(candle(11, 11, 10, 2));
    strategy.onData(candle(11.5, 11.5, 10.5, 3));
    strategy.onData(candle(10.4, 11, 10.4, 4));
    strategy.onData(candle(9.5, 10.5, 9.5, 5));

    REQUIRE(sink.orders.size() == 2);
    REQUIRE(sink.orders.back().side == Side::Sell);
    REQUIRE(strategy.signals().size() == 2);
    REQUIRE(strategy.signals().back().type == BreakoutSignalType::Sell);
}

TEST_CASE("BreakoutStrategy can short when enabled") {
    BreakoutStrategy strategy(/*lookback*/ 3, 0.0, 1.0, true);
    RecordingSink sink;
    strategy.setOrderSink(&sink);
    strategy.onStart();

    strategy.onData(candle(10, 10, 9.5, 0));
    strategy.onData(candle(10, 10, 9.5, 1));
    strategy.onData(candle(10, 10, 9.5, 2));
    strategy.onData(candle(9.5, 10, 9.5, 3));
    strategy.onData(candle(9.0, 9.5, 9.0, 4));

    REQUIRE(sink.orders.size() == 1);
    REQUIRE(sink.orders.front().side == Side::Sell);
    REQUIRE(strategy.signals().front().type == BreakoutSignalType::Sell);
}
