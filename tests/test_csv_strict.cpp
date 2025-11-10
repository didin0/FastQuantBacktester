#include <catch2/catch.hpp>
#include "../src/DataLoader/CSVDataLoader.h"

using namespace fastquant;

TEST_CASE("CSVDataLoader strict mode throws on malformed rows", "csv_strict") {
    // Use the sample CSV which contains a malformed row
    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = true; // strict -> should throw on malformed numeric field

    REQUIRE_THROWS_AS(loader.load("../examples/sample_prices.csv", cfg), std::exception);
}
