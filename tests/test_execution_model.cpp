#include <catch2/catch.hpp>
#include "../src/BacktestEngine/BacktestEngine.h"
#include "../src/Strategy/Strategy.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

using namespace fastquant;

namespace {

struct CandleRow {
    const char* ts;
    double open;
    double high;
    double low;
    double close;
    double volume;
    const char* symbol;
};

std::filesystem::path writeCsv(const std::vector<CandleRow>& rows, const std::string& name) {
    auto tmp = std::filesystem::temp_directory_path() / name;
    std::ofstream ofs(tmp);
    ofs << "timestamp,open,high,low,close,volume,symbol\n";
    for (const auto& row : rows) {
        ofs << row.ts << ','
            << row.open << ','
            << row.high << ','
            << row.low << ','
            << row.close << ','
            << row.volume << ','
            << row.symbol << "\n";
    }
    return tmp;
}

class SingleMarketOrderStrategy : public Strategy {
public:
    explicit SingleMarketOrderStrategy(double slippageBps = 0.0)
        : slippageBps_(slippageBps) {}

    void onStart() override { placed_ = false; }

    void onData(const Candle& candle) override {
        if (placed_ || !orderSink_) return;
        Order o;
        o.side = Side::Buy;
        o.type = OrderType::Market;
        o.qty = 2.0;
        o.symbol = candle.symbol;
        o.timestamp = candle.timestamp;
        o.slippageBps = slippageBps_;
        orderSink_->submit(o);
        placed_ = true;
    }

private:
    bool placed_{false};
    double slippageBps_{0.0};
};

class LimitSellStrategy : public Strategy {
public:
    LimitSellStrategy(double price, TimeInForce tif)
        : price_(price), tif_(tif) {}

    void onStart() override { placed_ = false; }

    void onData(const Candle& candle) override {
        if (placed_ || !orderSink_) return;
        Order o;
        o.side = Side::Sell;
        o.type = OrderType::Limit;
        o.price = price_;
        o.qty = 1.0;
        o.symbol = candle.symbol;
        o.timestamp = candle.timestamp;
        o.tif = tif_;
        orderSink_->submit(o);
        placed_ = true;
    }

private:
    double price_;
    TimeInForce tif_;
    bool placed_{false};
};

} // namespace

TEST_CASE("Market orders use candle open price with slippage and fees", "execution") {
    auto csv = writeCsv({
        {"2025-01-01T00:00:00Z", 100.0, 101.0, 99.0, 100.5, 10.0, "TEST"}
    }, "fqbt_exec_market.csv");

    ExecutionConfig execCfg;
    execCfg.defaultSlippageBps = 0.0;
    execCfg.commissionPerShare = 0.1;
    execCfg.commissionBps = 10.0; // 0.1%
    BacktestEngine engine(execCfg);

    SingleMarketOrderStrategy strat(50.0); // 0.5% per-order slippage

    CSVDataLoader::Config cfg;
    auto result = engine.run(csv.string(), cfg, strat, 100000.0);

    REQUIRE(result.trades.size() == 1);
    const auto& tr = result.trades.front();
    REQUIRE(tr.price == Approx(100.0 * 1.005));
    double expectedFee = 0.1 * 2.0 + 0.001 * tr.price * tr.qty;
    REQUIRE(tr.fee == Approx(expectedFee));
    REQUIRE(result.totalFees == Approx(expectedFee));
    double rawPrice = 100.0;
    REQUIRE(result.totalSlippage == Approx((tr.price - rawPrice) * tr.qty));
    std::filesystem::remove(csv);
}

TEST_CASE("Limit orders respect OHLC ranges", "execution") {
    auto csv = writeCsv({
        {"2025-01-01T00:00:00Z", 100.0, 101.0, 99.0, 100.5, 10.0, "TEST"},
        {"2025-01-01T01:00:00Z", 104.0, 106.0, 103.0, 105.0, 10.0, "TEST"}
    }, "fqbt_exec_limit.csv");

    BacktestEngine engine;
    LimitSellStrategy strat(105.0, TimeInForce::GTC);

    CSVDataLoader::Config cfg;
    auto result = engine.run(csv.string(), cfg, strat, 100000.0);

    REQUIRE(result.trades.size() == 1);
    REQUIRE(result.trades.front().price == Approx(105.0));
    REQUIRE(result.ordersFilled == 1);
    REQUIRE(result.ordersRejected == 0);
    std::filesystem::remove(csv);
}

TEST_CASE("IOC limit orders expire when not filled", "execution") {
    auto csv = writeCsv({
        {"2025-01-01T00:00:00Z", 100.0, 101.0, 99.0, 100.5, 10.0, "TEST"}
    }, "fqbt_exec_ioc.csv");

    BacktestEngine engine;
    LimitSellStrategy strat(110.0, TimeInForce::IOC);

    CSVDataLoader::Config cfg;
    auto result = engine.run(csv.string(), cfg, strat, 100000.0);

    REQUIRE(result.trades.empty());
    REQUIRE(result.ordersFilled == 0);
    REQUIRE(result.ordersRejected == 1);
    std::filesystem::remove(csv);
}
