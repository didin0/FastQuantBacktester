#include <catch2/catch.hpp>
#include "../src/Model/Portfolio.h"

using namespace fastquant;

static Trade make_trade(Side side, double price, double qty, const std::string& symbol = "TEST") {
    static int counter = 0;
    Trade t;
    t.id = "t" + std::to_string(counter++);
    t.orderId = t.id;
    t.side = side;
    t.price = price;
    t.qty = qty;
    t.symbol = symbol;
    t.timestamp = std::chrono::system_clock::now();
    return t;
}

TEST_CASE("Portfolio long position PnL", "portfolio") {
    Portfolio p(100000.0);

    p.applyTrade(make_trade(Side::Buy, 100.0, 10));
    REQUIRE(p.cash() == Approx(100000.0 - 1000.0));
    REQUIRE(p.realizedPnl() == Approx(0.0));
    REQUIRE(p.positions().at("TEST").qty == Approx(10.0));

    p.markPrice("TEST", 105.0);
    REQUIRE(p.unrealizedPnl() == Approx(50.0));
    REQUIRE(p.equity() == Approx(p.cash() + 10 * 105.0));

    p.applyTrade(make_trade(Side::Sell, 110.0, 5));
    REQUIRE(p.realizedPnl() == Approx(50.0));
    REQUIRE(p.positions().at("TEST").qty == Approx(5.0));
    REQUIRE(p.positions().at("TEST").avgPrice == Approx(100.0));

    p.applyTrade(make_trade(Side::Sell, 90.0, 5));
    REQUIRE(p.positions().at("TEST").qty == Approx(0.0));
    REQUIRE(p.realizedPnl() == Approx(0.0));
}

TEST_CASE("Portfolio short position", "portfolio") {
    Portfolio p(50000.0);
    p.applyTrade(make_trade(Side::Sell, 50.0, 4, "XYZ"));
    REQUIRE(p.cash() == Approx(50000.0 + 200.0));
    REQUIRE(p.positions().at("XYZ").qty == Approx(-4.0));
    REQUIRE(p.positions().at("XYZ").avgPrice == Approx(50.0));

    p.markPrice("XYZ", 45.0);
    REQUIRE(p.unrealizedPnl() == Approx((45.0 - 50.0) * -4.0)); // positive 20

    p.applyTrade(make_trade(Side::Buy, 48.0, 2, "XYZ"));
    REQUIRE(p.positions().at("XYZ").qty == Approx(-2.0));
    REQUIRE(p.realizedPnl() == Approx((50.0 - 48.0) * 2.0));
}
