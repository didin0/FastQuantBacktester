#include <catch2/catch.hpp>
#include "../src/BacktestEngine/BacktestEngine.h"
#include "../src/Strategy/MovingAverageStrategy.h"
#include "../src/Reporter/Reporter.h"
#include <filesystem>
#include <fstream>

using namespace fastquant;

TEST_CASE("Reporter generates summaries and files", "reporter") {
    MovingAverageStrategy strat(3, 5);
    BacktestEngine engine;

    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    std::filesystem::path sample = std::filesystem::path(__FILE__).parent_path().parent_path() / "examples" / "sample_prices.csv";
    auto result = engine.run(sample.string(), cfg, strat, 100000.0);

    Reporter reporter;
    auto summary = reporter.summarize(result);
    REQUIRE(summary.initialCapital == Approx(100000.0));
    REQUIRE(summary.trades == result.trades.size());
    REQUIRE(summary.finalEquity > 0.0);

    auto tmpdir = std::filesystem::temp_directory_path();
    auto jsonPath = tmpdir / "report_test.json";
    auto csvPath = tmpdir / "report_summary.csv";
    auto tradesCsv = tmpdir / "report_trades.csv";

    reporter.writeJson(result, jsonPath.string());
    reporter.writeSummaryCsv(result, csvPath.string());
    reporter.writeTradesCsv(result, tradesCsv.string());

    REQUIRE(std::filesystem::exists(jsonPath));
    REQUIRE(std::filesystem::exists(csvPath));
    REQUIRE(std::filesystem::exists(tradesCsv));

    REQUIRE(std::filesystem::file_size(jsonPath) > 0);
    REQUIRE(std::filesystem::file_size(csvPath) > 0);
    REQUIRE(std::filesystem::file_size(tradesCsv) > 0);

    // Clean up temp files
    std::filesystem::remove(jsonPath);
    std::filesystem::remove(csvPath);
    std::filesystem::remove(tradesCsv);
}
