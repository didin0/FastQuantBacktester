#include <catch2/catch.hpp>
#include "../src/DataLoader/CSVDataLoader.h"

using namespace fastquant;

TEST_CASE("CSVDataLoader.stream early-stop works", "csv_stream_earlystop") {
    auto sample = std::string("../examples/sample_prices.csv");
    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    size_t count = 0;
    // Stop after first valid candle
    loader.stream(sample, [&](const Candle& c)->bool {
        ++count;
        return false; // request early stop
    }, cfg);

    REQUIRE(count == 1);
}
