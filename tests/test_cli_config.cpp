#include <catch2/catch.hpp>
#include "../src/App/RunConfig.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iterator>
#include <string>
#include <system_error>

using fastquant::app::RunConfig;

namespace {
std::filesystem::path makeTempDir() {
    auto base = std::filesystem::temp_directory_path();
    for (int i = 0; i < 32; ++i) {
        auto candidate = base / std::filesystem::path("fqbt-cli-" + std::to_string(std::rand()));
        if (!std::filesystem::exists(candidate)) {
            std::filesystem::create_directories(candidate);
            return candidate;
        }
    }
    throw std::runtime_error("Unable to create temp directory for CLI test");
}

void writeSampleCsv(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    ofs << "timestamp,open,high,low,close,volume,symbol\n";
    ofs << "2024-01-01T00:00:00Z,100,101,99,100,1000,TEST\n";
    ofs << "2024-01-02T00:00:00Z,100,102,98,101,1000,TEST\n";
    ofs << "2024-01-03T00:00:00Z,101,103,100,102,1000,TEST\n";
    ofs << "2024-01-04T00:00:00Z,103,104,101,99,1000,TEST\n";
    ofs << "2024-01-05T00:00:00Z,99,100,97,98,1000,TEST\n";
}

void writeConfig(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    ofs << R"({
  "data": {
    "path": "data.csv",
    "has_header": true,
    "strict": true
  },
  "strategy": {
    "type": "moving_average",
    "short_window": 2,
    "long_window": 3
  },
  "engine": {
    "initial_capital": 50000
  },
  "reporter": {
    "json": "reports/out.json",
    "summary_csv": "reports/summary.csv",
    "trades_csv": "reports/trades.csv",
    "print_summary": false
  }
})";
}

} // namespace

TEST_CASE("RunConfig drives CLI pipeline", "[cli][config]") {
    auto tmp = makeTempDir();
    auto csv = tmp / "data.csv";
    auto cfgPath = tmp / "config.json";
    writeSampleCsv(csv);
    writeConfig(cfgPath);

    auto cfg = fastquant::app::loadRunConfig(cfgPath.string());
    REQUIRE(std::filesystem::path(cfg.dataPath) == std::filesystem::weakly_canonical(csv));
    REQUIRE(cfg.initialCapital == Approx(50000));

    auto result = fastquant::app::executeBacktest(cfg);
    REQUIRE(result.candlesProcessed == 5);

    auto summary = fastquant::app::generateReports(cfg, result);
    REQUIRE(summary.initialCapital == Approx(50000));

    auto jsonReport = tmp / "reports" / "out.json";
    auto summaryCsv = tmp / "reports" / "summary.csv";
    auto tradesCsv = tmp / "reports" / "trades.csv";

    REQUIRE(std::filesystem::exists(jsonReport));
    REQUIRE(std::filesystem::exists(summaryCsv));
    REQUIRE(std::filesystem::exists(tradesCsv));

    std::ifstream ifs(jsonReport);
    std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    REQUIRE(contents.find("summary") != std::string::npos);

  std::error_code ec;
  std::filesystem::remove_all(tmp, ec);
}
