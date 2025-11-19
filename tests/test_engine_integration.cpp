#include <catch2/catch.hpp>
#include "../src/BacktestEngine/BacktestEngine.h"
#include "../src/Strategy/MovingAverageStrategy.h"
#include <filesystem>
#include <functional>

using namespace fastquant;

TEST_CASE("BacktestEngine runs strategy and captures trades", "engine") {
    MovingAverageStrategy strat(3,5);
    BacktestEngine engine;

    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    // Determine sample CSV path relative to source file location so tests work from build dir.
    std::filesystem::path sample = std::filesystem::path(__FILE__).parent_path().parent_path() / "examples" / "sample_prices.csv";
    auto result = engine.run(sample.string(), cfg, strat, 100000.0);

    // We expect to have processed at least a few candles and captured trades corresponding to signals
    REQUIRE(result.candlesProcessed >= 3);
    REQUIRE(result.trades.size() == strat.signals().size());
    if (!result.trades.empty()) {
        REQUIRE(result.trades.front().price > 0.0);
    }
    REQUIRE(result.ordersFilled == result.trades.size());
    REQUIRE(result.ordersRejected >= 0);

    // Portfolio sanity checks
    REQUIRE(result.portfolio.cash() <= 100000.0);
    REQUIRE(result.portfolio.equity() > 0.0);
}

TEST_CASE("BacktestEngine runs multiple strategies in parallel", "engine") {
    BacktestEngine engine;

    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    std::filesystem::path sample = std::filesystem::path(__FILE__).parent_path().parent_path() / "examples" / "sample_prices.csv";

    std::vector<std::function<std::unique_ptr<Strategy>()>> factories;
    factories.emplace_back([]() { return std::make_unique<MovingAverageStrategy>(3, 5); });
    factories.emplace_back([]() { return std::make_unique<MovingAverageStrategy>(2, 4); });

    auto results = engine.runParallel(sample.string(), cfg, factories, 100000.0);
    REQUIRE(results.size() == factories.size());
    for (const auto& result : results) {
        REQUIRE(result.candlesProcessed >= 3);
        REQUIRE(result.ordersFilled == result.trades.size());
    }
}
