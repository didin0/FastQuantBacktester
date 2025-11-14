#pragma once

#include "../DataLoader/CSVDataLoader.h"
#include "../BacktestEngine/BacktestEngine.h"
#include "../Reporter/Reporter.h"
#include <optional>
#include <string>

namespace fastquant {
namespace app {

struct StrategyConfig {
    std::string type = "moving_average";
    size_t shortWindow = 5;
    size_t longWindow = 20;
};

struct ReporterOutputs {
    std::optional<std::string> jsonPath;
    std::optional<std::string> summaryCsvPath;
    std::optional<std::string> tradesCsvPath;
    bool printSummary = true;
};

struct RunConfig {
    std::string sourceConfigPath; // absolute path to the JSON file
    std::string dataPath;         // absolute path to CSV input
    CSVDataLoader::Config loaderConfig;
    StrategyConfig strategy;
    double initialCapital = 100000.0;
    ReporterOutputs outputs;
};

// Load a RunConfig from disk. Throws std::runtime_error on invalid JSON or missing fields.
RunConfig loadRunConfig(const std::string& path);

// Execute the backtest described by cfg, returning the BacktestResult produced by BacktestEngine.
BacktestResult executeBacktest(const RunConfig& cfg);

// Generate all configured reports (JSON / CSV) and return the computed summary.
ReportSummary generateReports(const RunConfig& cfg, const BacktestResult& result);

} // namespace app
} // namespace fastquant
