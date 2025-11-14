#include <catch2/catch.hpp>
#include "../src/BacktestEngine/BacktestEngine.h"
#include "../src/Strategy/MovingAverageStrategy.h"
#include <filesystem>

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
    // Since MovingAverageStrategy emits orders on signals, trades should match signals count
    REQUIRE(result.trades.size() == strat.signals().size());
    if (!result.trades.empty()) {
        REQUIRE(result.trades.front().price == strat.signals().front().price);
    }

    // Portfolio sanity checks
    REQUIRE(result.portfolio.cash() <= 100000.0);
    REQUIRE(result.portfolio.equity() > 0.0);
}
