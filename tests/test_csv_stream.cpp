#include <catch2/catch.hpp>
#include "../src/DataLoader/Candle.h"
#include "../src/DataLoader/CSVDataLoader.h"
#include <filesystem>

using namespace fastquant;

TEST_CASE("CSVDataLoader.stream invokes callback for each valid candle", "csv_stream") {
    auto sample = std::filesystem::path("../examples/sample_prices.csv");
    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    size_t count = 0;
    loader.stream(sample.string(), [&](const Candle& c)->bool{
        ++count;
        // quick sanity check
        REQUIRE(c.close >= 0.0);
        return true; // continue
    }, cfg);

    REQUIRE(count == 3);
}
