#include <catch2/catch.hpp>
#include "../src/DataLoader/CSVDataLoader.h"
#include <chrono>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <iostream>

using namespace fastquant;

// Performance benchmark for CSVDataLoader streaming path.
TEST_CASE("CSVDataLoader performance benchmark (stream)", "bench") {
    // Skip the perf test unless ENABLE_PERF=1 to keep CI fast.
    if (const char* en = std::getenv("ENABLE_PERF")) {
        std::string s(en);
        if (s != "1") {
            INFO("Performance test skipped (ENABLE_PERF != 1)");
            return;
        }
    } else {
        INFO("Performance test skipped (ENABLE_PERF not set)");
        return;
    }

    // Number of rows can be overridden with env var FQ_BENCH_ROWS. Default to 100000.
    size_t rows = 100000;
    if (const char* env = std::getenv("FQ_BENCH_ROWS")) {
        try { rows = static_cast<size_t>(std::stoul(env)); } catch(...) {}
    }

    std::filesystem::path out = std::filesystem::current_path() / "perf_generated.csv";

    // Generate CSV if missing or too small
    bool need_gen = true;
    if (std::filesystem::exists(out)) {
        try {
            auto sz = std::filesystem::file_size(out);
            if (sz > 0) need_gen = false;
        } catch(...) { need_gen = true; }
    }

    if (need_gen) {
        std::ofstream ofs(out);
        ofs << "timestamp,open,high,low,close,volume\n";
        // simple synthetic data
        std::tm tm = {};
        tm.tm_year = 125; // year 2025
        tm.tm_mon = 10; // November (0-based)
        tm.tm_mday = 8;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        time_t base = std::mktime(&tm);
        for (size_t i = 0; i < rows; ++i) {
            time_t t = base + static_cast<time_t>(i * 60); // per-minute candles
            std::tm *ptm = std::gmtime(&t);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", ptm);
            double o = 10000.0 + (i % 100) * 0.1;
            double h = o + 1.0;
            double l = o - 1.0;
            double c = o + 0.5;
            double v = 1.0 + (i % 10);
            ofs << buf << ',' << o << ',' << h << ',' << l << ',' << c << ',' << v << '\n';
        }
        ofs.close();
    }

    CSVDataLoader loader;
    CSVDataLoader::Config cfg;
    cfg.hasHeader = true;
    cfg.strict = false;

    size_t count = 0;
    auto t0 = std::chrono::steady_clock::now();
    loader.stream(out.string(), [&](const Candle& c)->bool {
        ++count;
        return true;
    }, cfg);
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;

    // Print benchmark info to stdout (Catch2 will show it in test output)
    std::cout << "CSV stream processed " << count << " rows in " << elapsed.count() << "s\n";

    REQUIRE(count == rows);
}
