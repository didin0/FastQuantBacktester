#pragma once

#include "../DataLoader/CSVDataLoader.h"
#include "../DataLoader/APIDataLoader.h"
#include "../BacktestEngine/BacktestEngine.h"
#include "../Reporter/Reporter.h"
#include <optional>
#include <string>
#include <vector>

namespace fastquant {
namespace app {

struct StrategyConfig {
    std::string name = "strategy";
    std::string type = "moving_average";
    size_t shortWindow = 5;
    size_t longWindow = 20;
    size_t breakoutLookback = 20;
    double breakoutBuffer = 0.0;
    double orderQuantity = 1.0;
    bool allowShort = false;
};

struct ReporterOutputs {
    std::optional<std::string> jsonPath;
    std::optional<std::string> summaryCsvPath;
    std::optional<std::string> tradesCsvPath;
    bool printSummary = true;
};

enum class DataSourceKind { CSV, API };

struct RunConfig {
    std::string sourceConfigPath; // absolute path to the JSON file
    std::string dataPath;         // absolute path to CSV input
    CSVDataLoader::Config loaderConfig;
    DataSourceKind dataSource = DataSourceKind::CSV;
    std::optional<APIDataConfig> apiData;
    StrategyConfig strategy;
    std::vector<StrategyConfig> strategies;
    double initialCapital = 100000.0;
    ReporterOutputs outputs;
    ExecutionConfig execution;
};

struct StrategyRunResult {
    StrategyConfig config;
    BacktestResult result;
};

struct StrategyReport {
    StrategyConfig config;
    ReportSummary summary;
    ReporterOutputs outputs;
};

// Load a RunConfig from disk. Throws std::runtime_error on invalid JSON or missing fields.
RunConfig loadRunConfig(const std::string& path);

// Execute the backtest described by cfg, returning the BacktestResult produced by BacktestEngine.
BacktestResult executeBacktest(const RunConfig& cfg);
std::vector<StrategyRunResult> executeBacktests(const RunConfig& cfg);

// Generate all configured reports (JSON / CSV) and return the computed summary.
ReportSummary generateReports(const RunConfig& cfg, const BacktestResult& result);
std::vector<StrategyReport> generateReports(const RunConfig& cfg, const std::vector<StrategyRunResult>& runs);

} // namespace app
} // namespace fastquant
