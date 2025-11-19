#include <catch2/catch.hpp>
#include "../src/App/RunConfig.h"
#include "../src/App/EnvLoader.h"
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

void writeApiConfig(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    ofs << R"({
  "data": {
    "source": "api",
    "endpoint": "https://example.com/api",
    "symbol": "TEST",
    "headers": {"X-API-KEY": "demo"},
    "query": {"limit": "5"},
    "data_field": "bars",
    "fields": {
      "timestamp": "ts",
      "open": "o",
      "high": "h",
      "low": "l",
      "close": "c",
      "volume": "v",
      "symbol": "ticker"
    }
  },
  "strategy": {
    "type": "moving_average",
    "short_window": 2,
    "long_window": 3
  }
})";
}

void writeMultiStrategyConfig(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    ofs << R"({
  "data": {
    "path": "data.csv",
    "has_header": true,
    "strict": true
  },
  "strategies": [
    {
      "name": "sma_fast",
      "type": "moving_average",
      "short_window": 2,
      "long_window": 3
    },
    {
      "name": "breakout_20",
      "type": "breakout",
      "breakout_lookback": 20,
      "breakout_buffer": 0.25,
      "order_quantity": 10,
      "allow_short": true
    }
  ],
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

TEST_CASE("Parallel strategy configs run and produce suffixed reports", "[cli][config][parallel]") {
    auto tmp = makeTempDir();
    auto csv = tmp / "data.csv";
    auto cfgPath = tmp / "multi.json";
    writeSampleCsv(csv);
    writeMultiStrategyConfig(cfgPath);

    auto cfg = fastquant::app::loadRunConfig(cfgPath.string());
    REQUIRE(cfg.strategies.size() == 2);
    REQUIRE(cfg.strategies[0].name == "sma_fast");
    REQUIRE(cfg.strategies[1].name == "breakout_20");

    auto runs = fastquant::app::executeBacktests(cfg);
    REQUIRE(runs.size() == 2);

    auto reports = fastquant::app::generateReports(cfg, runs);
    REQUIRE(reports.size() == 2);

    auto reportDir = tmp / "reports";
  auto expectJson1 = std::filesystem::weakly_canonical(reportDir / "out_sma_fast_1.json");
  auto expectJson2 = std::filesystem::weakly_canonical(reportDir / "out_breakout_20_2.json");
    REQUIRE(reports[0].outputs.jsonPath);
    REQUIRE(reports[1].outputs.jsonPath);
    REQUIRE(std::filesystem::weakly_canonical(*reports[0].outputs.jsonPath) == expectJson1);
    REQUIRE(std::filesystem::weakly_canonical(*reports[1].outputs.jsonPath) == expectJson2);
    REQUIRE(std::filesystem::exists(expectJson1));
    REQUIRE(std::filesystem::exists(expectJson2));

    std::error_code ec;
    std::filesystem::remove_all(tmp, ec);
}

TEST_CASE("RunConfig parses API data source", "[cli][config][api]") {
  auto tmp = makeTempDir();
  auto cfgPath = tmp / "api.json";
  writeApiConfig(cfgPath);

  auto cfg = fastquant::app::loadRunConfig(cfgPath.string());
  REQUIRE(cfg.dataSource == fastquant::app::DataSourceKind::API);
  REQUIRE(cfg.apiData);
  REQUIRE(cfg.apiData->endpoint == "https://example.com/api");
  REQUIRE(cfg.apiData->headers.size() == 1);
  REQUIRE(cfg.apiData->query.size() == 1);
  REQUIRE(cfg.apiData->dataField);

  std::error_code ec;
  std::filesystem::remove_all(tmp, ec);
}

TEST_CASE("Environment placeholders expand inside configs", "[cli][config][env]") {
    auto tmp = makeTempDir();
    auto envPath = tmp / ".env";
    {
        std::ofstream ofs(envPath);
        ofs << "BINANCE_KEY=secret123";
    }

    auto cfgPath = tmp / "api_env.json";
    std::ofstream ofs(cfgPath);
    ofs << R"({
  "data": {
    "source": "api",
    "endpoint": "https://example.com/api",
    "headers": {"X-MBX-APIKEY": "${BINANCE_KEY}"},
    "fields": {"timestamp": "ts", "open": "o", "high": "h", "low": "l", "close": "c", "volume": "v"}
  }
})";
    ofs.close();

    fastquant::app::loadEnvFile(envPath);
    auto cfg = fastquant::app::loadRunConfig(cfgPath.string());
    REQUIRE(cfg.apiData);
    REQUIRE_FALSE(cfg.apiData->headers.empty());
    REQUIRE(cfg.apiData->headers[0].second == "secret123");

    std::error_code ec;
    std::filesystem::remove_all(tmp, ec);
}
