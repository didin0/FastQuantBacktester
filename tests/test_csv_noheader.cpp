#include <catch2/catch.hpp>
#include "../src/DataLoader/CSVDataLoader.h"

using namespace fastquant;

TEST_CASE("CSVDataLoader loads headerless CSV when configured", "csv_noheader") {
    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = false; // file has no header
    cfg.strict = false;

    auto candles = loader.load("../examples/sample_noheader.csv", cfg);
    // 2 valid rows, 1 malformed -> expect 2
    REQUIRE(candles.size() == 2);
    REQUIRE(candles[0].symbol == "BTCUSD");
}
