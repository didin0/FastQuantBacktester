#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "../src/DataLoader/Candle.h"
#include "../src/DataLoader/CSVDataLoader.h"
#include <filesystem>

using namespace fastquant;

TEST_CASE("CSVDataLoader parses happy path and skips malformed rows", "csv") {
    auto sample = std::filesystem::path("../examples/sample_prices.csv");
    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    auto candles = loader.load(sample.string(), cfg);

    // The sample CSV contains 3 valid rows and 1 malformed row -> expect 3 candles
    REQUIRE(candles.size() == 3);

    REQUIRE(candles[0].open == Approx(60000.0));
    REQUIRE(candles[0].symbol == "BTCUSD");
}
