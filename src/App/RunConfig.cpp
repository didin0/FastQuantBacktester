#include "RunConfig.h"
#include "../Strategy/MovingAverageStrategy.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace fastquant {
namespace app {
namespace {

std::string resolvePath(const std::filesystem::path& baseDir, const std::string& maybeRelative) {
    if (maybeRelative.empty()) {
        return maybeRelative;
    }
    std::filesystem::path p(maybeRelative);
    if (p.is_absolute()) {
        return std::filesystem::weakly_canonical(p).string();
    }
    auto combined = std::filesystem::weakly_canonical(baseDir / p);
    return combined.string();
}

CSVDataLoader::Config parseLoaderConfig(const nlohmann::json& dataSection) {
    CSVDataLoader::Config loader;
    if (auto delimIt = dataSection.find("delimiter"); delimIt != dataSection.end()) {
        std::string delim = delimIt->get<std::string>();
        if (!delim.empty()) {
            loader.delimiter = delim.front();
        }
    }
    loader.hasHeader = dataSection.value("has_header", loader.hasHeader);
    loader.strict = dataSection.value("strict", loader.strict);
    loader.timestampFormat = dataSection.value("timestamp_format", loader.timestampFormat);
    return loader;
}

StrategyConfig parseStrategyConfig(const nlohmann::json& stratSection) {
    StrategyConfig cfg;
    if (!stratSection.is_object()) {
        return cfg;
    }
    cfg.type = stratSection.value("type", cfg.type);
    cfg.shortWindow = stratSection.value("short_window", cfg.shortWindow);
    cfg.longWindow = stratSection.value("long_window", cfg.longWindow);
    if (cfg.shortWindow == 0) cfg.shortWindow = 1;
    if (cfg.longWindow == 0) cfg.longWindow = 1;
    return cfg;
}

ReporterOutputs parseReporterOutputs(const nlohmann::json& reportSection, const std::filesystem::path& baseDir) {
    ReporterOutputs outputs;
    if (!reportSection.is_object()) {
        return outputs;
    }
    outputs.printSummary = reportSection.value("print_summary", outputs.printSummary);
    if (auto it = reportSection.find("json"); it != reportSection.end() && it->is_string()) {
        outputs.jsonPath = resolvePath(baseDir, it->get<std::string>());
    }
    if (auto it = reportSection.find("summary_csv"); it != reportSection.end() && it->is_string()) {
        outputs.summaryCsvPath = resolvePath(baseDir, it->get<std::string>());
    }
    if (auto it = reportSection.find("trades_csv"); it != reportSection.end() && it->is_string()) {
        outputs.tradesCsvPath = resolvePath(baseDir, it->get<std::string>());
    }
    return outputs;
}

std::unique_ptr<Strategy> buildStrategy(const StrategyConfig& cfg) {
    if (cfg.type == "moving_average") {
        return std::make_unique<MovingAverageStrategy>(cfg.shortWindow, cfg.longWindow);
    }
    std::ostringstream oss;
    oss << "Unsupported strategy type: " << cfg.type;
    throw std::runtime_error(oss.str());
}

void ensureParentDirectory(const std::string& path) {
    if (path.empty()) return;
    std::filesystem::path p(path);
    auto parent = p.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

} // namespace

RunConfig loadRunConfig(const std::string& path) {
    std::filesystem::path absPath = std::filesystem::weakly_canonical(std::filesystem::path(path));
    if (!std::filesystem::exists(absPath)) {
        throw std::runtime_error("Config file not found: " + absPath.string());
    }

    std::ifstream ifs(absPath);
    if (!ifs.good()) {
        throw std::runtime_error("Failed to open config file: " + absPath.string());
    }

    nlohmann::json j;
    ifs >> j;

    RunConfig cfg;
    cfg.sourceConfigPath = absPath.string();

    if (!j.contains("data")) {
        throw std::runtime_error("Config missing 'data' section");
    }
    const auto& dataSection = j.at("data");
    if (!dataSection.contains("path")) {
        throw std::runtime_error("Config missing data.path");
    }
    std::filesystem::path baseDir = absPath.parent_path();
    cfg.dataPath = resolvePath(baseDir, dataSection.at("path").get<std::string>());
    if (!std::filesystem::exists(cfg.dataPath)) {
        throw std::runtime_error("CSV data file not found: " + cfg.dataPath);
    }
    cfg.loaderConfig = parseLoaderConfig(dataSection);

    cfg.strategy = parseStrategyConfig(j.value("strategy", nlohmann::json::object()));
    cfg.initialCapital = j.value("engine", nlohmann::json::object()).value("initial_capital", cfg.initialCapital);
    cfg.outputs = parseReporterOutputs(j.value("reporter", nlohmann::json::object()), baseDir);
    return cfg;
}

BacktestResult executeBacktest(const RunConfig& cfg) {
    BacktestEngine engine;
    auto strategy = buildStrategy(cfg.strategy);
    return engine.run(cfg.dataPath, cfg.loaderConfig, *strategy, cfg.initialCapital);
}

ReportSummary generateReports(const RunConfig& cfg, const BacktestResult& result) {
    Reporter reporter;
    auto summary = reporter.summarize(result);

    if (cfg.outputs.jsonPath) {
        ensureParentDirectory(*cfg.outputs.jsonPath);
        reporter.writeJson(result, *cfg.outputs.jsonPath);
    }
    if (cfg.outputs.summaryCsvPath) {
        ensureParentDirectory(*cfg.outputs.summaryCsvPath);
        reporter.writeSummaryCsv(result, *cfg.outputs.summaryCsvPath);
    }
    if (cfg.outputs.tradesCsvPath) {
        ensureParentDirectory(*cfg.outputs.tradesCsvPath);
        reporter.writeTradesCsv(result, *cfg.outputs.tradesCsvPath);
    }

    return summary;
}

} // namespace app
} // namespace fastquant
