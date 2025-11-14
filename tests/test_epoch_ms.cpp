#include <catch2/catch.hpp>
#include "../src/DataLoader/CSVDataLoader.h"
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace fastquant;

TEST_CASE("CSVDataLoader parses epoch-ms and epoch-sec timestamps", "timestamp") {
    // Create a small CSV in the build/test working directory
    std::filesystem::path out = std::filesystem::current_path() / "epoch_test.csv";
    std::ofstream ofs(out);
    REQUIRE(ofs.good());
    ofs << "timestamp,open,high,low,close,volume\n";

    // 1609459200000 ms => 2021-01-01T00:00:00Z
    long long ms = 1609459200000LL;
    // 1609459260 sec => 2021-01-01T00:01:00Z
    long long sec = 1609459260LL;

    ofs << ms << ",100,101,99,100,1\n";
    ofs << sec << ",100,101,99,101,1\n";
    ofs.close();

    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = true;

    std::vector<Candle> rows;
    loader.stream(out.string(), [&](const Candle& c)->bool {
        rows.push_back(c);
        return true;
    }, cfg);

    REQUIRE(rows.size() == 2);

    auto ms0 = std::chrono::duration_cast<std::chrono::milliseconds>(rows[0].timestamp.time_since_epoch()).count();
    auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(rows[1].timestamp.time_since_epoch()).count();

    REQUIRE(ms0 == ms);
    REQUIRE(ms1 == sec * 1000LL);
}
