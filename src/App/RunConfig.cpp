#include "RunConfig.h"
#include "EnvLoader.h"
#include "../Strategy/BreakoutStrategy.h"
#include "../Strategy/MovingAverageStrategy.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <cctype>
#include <functional>

namespace fastquant {
namespace app {
namespace {

std::string resolvePath(const std::filesystem::path& baseDir, const std::string& maybeRelative) {
    if (maybeRelative.empty()) {
        return maybeRelative;
    }
    auto expanded = expandEnvVars(maybeRelative);
    if (expanded.empty()) {
        return expanded;
    }
    std::filesystem::path p(expanded);
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
    cfg.name = stratSection.value("name", cfg.name);
    cfg.type = stratSection.value("type", cfg.type);
    cfg.shortWindow = stratSection.value("short_window", cfg.shortWindow);
    cfg.longWindow = stratSection.value("long_window", cfg.longWindow);
    cfg.breakoutLookback = stratSection.value("breakout_lookback", cfg.breakoutLookback);
    cfg.breakoutBuffer = stratSection.value("breakout_buffer", cfg.breakoutBuffer);
    cfg.orderQuantity = stratSection.value("order_quantity", cfg.orderQuantity);
    cfg.allowShort = stratSection.value("allow_short", cfg.allowShort);
    if (cfg.shortWindow == 0) cfg.shortWindow = 1;
    if (cfg.longWindow == 0) cfg.longWindow = 1;
    if (cfg.breakoutLookback == 0) cfg.breakoutLookback = 1;
    if (cfg.name.empty()) {
        cfg.name = cfg.type;
    }
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

ExecutionConfig parseExecutionConfig(const nlohmann::json& execSection) {
    ExecutionConfig exec;
    if (!execSection.is_object()) {
        return exec;
    }
    exec.defaultSlippageBps = execSection.value("default_slippage_bps", exec.defaultSlippageBps);
    exec.commissionPerShare = execSection.value("commission_per_share", exec.commissionPerShare);
    exec.commissionBps = execSection.value("commission_bps", exec.commissionBps);
    return exec;
}

std::vector<std::pair<std::string, std::string>> parseStringPairs(const nlohmann::json& node) {
    std::vector<std::pair<std::string, std::string>> out;
    if (!node.is_object()) {
        return out;
    }
    for (auto it = node.begin(); it != node.end(); ++it) {
        if (it->is_string()) {
            out.emplace_back(it.key(), expandEnvVars(it->get<std::string>()));
        } else {
            out.emplace_back(it.key(), expandEnvVars(it->dump()));
        }
    }
    return out;
}

APIDataConfig parseApiDataConfig(const nlohmann::json& dataSection) {
    if (!dataSection.contains("endpoint")) {
        throw std::runtime_error("API data source requires 'endpoint'");
    }
    APIDataConfig cfg;
    cfg.endpoint = expandEnvVars(dataSection.at("endpoint").get<std::string>());
    if (auto symIt = dataSection.find("symbol"); symIt != dataSection.end() && symIt->is_string()) {
        cfg.fallbackSymbol = expandEnvVars(symIt->get<std::string>());
    }
    if (auto it = dataSection.find("headers"); it != dataSection.end()) {
        cfg.headers = parseStringPairs(*it);
    }
    if (auto it = dataSection.find("query"); it != dataSection.end()) {
        cfg.query = parseStringPairs(*it);
    }
    if (auto it = dataSection.find("data_field"); it != dataSection.end() && it->is_string()) {
        cfg.dataField = expandEnvVars(it->get<std::string>());
    }
    if (auto fields = dataSection.find("fields"); fields != dataSection.end() && fields->is_object()) {
        cfg.fields.timestamp = fields->value("timestamp", cfg.fields.timestamp);
        cfg.fields.open = fields->value("open", cfg.fields.open);
        cfg.fields.high = fields->value("high", cfg.fields.high);
        cfg.fields.low = fields->value("low", cfg.fields.low);
        cfg.fields.close = fields->value("close", cfg.fields.close);
        cfg.fields.volume = fields->value("volume", cfg.fields.volume);
        cfg.fields.symbol = fields->value("symbol", cfg.fields.symbol);
    }
    return cfg;
}

std::vector<StrategyConfig> parseStrategyList(const nlohmann::json& root) {
    std::vector<StrategyConfig> list;
    auto it = root.find("strategies");
    if (it == root.end() || !it->is_array()) {
        return list;
    }
    for (const auto& entry : *it) {
        list.push_back(parseStrategyConfig(entry));
    }
    return list;
}

void ensureStrategyNames(std::vector<StrategyConfig>& strategies) {
    for (size_t i = 0; i < strategies.size(); ++i) {
        if (strategies[i].name.empty()) {
            strategies[i].name = strategies[i].type + "-" + std::to_string(i + 1);
        }
    }
    std::unordered_map<std::string, size_t> seen;
    for (auto& strat : strategies) {
        auto& count = seen[strat.name];
        if (count > 0) {
            strat.name += "-" + std::to_string(count + 1);
        }
        ++count;
    }
}

std::string sanitizeName(const std::string& name) {
    std::string sanitized;
    sanitized.reserve(name.size());
    for (char ch : name) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            sanitized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        } else if (!sanitized.empty() && sanitized.back() == '_') {
            continue;
        } else {
            sanitized.push_back('_');
        }
    }
    if (sanitized.empty()) {
        sanitized = "strategy";
    }
    return sanitized;
}

std::string appendSuffix(const std::string& path, const std::string& suffix) {
    std::filesystem::path p(path);
    auto stem = p.stem().string();
    auto ext = p.extension().string();
    if (stem.empty()) {
        stem = p.filename().string();
        ext.clear();
    }
    auto newName = stem + "_" + suffix + ext;
    if (auto parent = p.parent_path(); !parent.empty()) {
        return (parent / newName).string();
    }
    return newName;
}

ReporterOutputs resolveOutputsForStrategy(const ReporterOutputs& base,
                                          const StrategyConfig& strategy,
                                          size_t index,
                                          size_t total) {
    ReporterOutputs resolved = base;
    if (total <= 1) {
        return resolved;
    }
    std::string suffix = sanitizeName(!strategy.name.empty() ? strategy.name : ("strategy_" + std::to_string(index + 1)));
    suffix += "_" + std::to_string(index + 1);
    auto apply = [&](std::optional<std::string>& path) {
        if (path) {
            path = appendSuffix(*path, suffix);
        }
    };
    apply(resolved.jsonPath);
    apply(resolved.summaryCsvPath);
    apply(resolved.tradesCsvPath);
    return resolved;
}

std::unique_ptr<Strategy> buildStrategy(const StrategyConfig& cfg) {
    if (cfg.type == "moving_average") {
        return std::make_unique<MovingAverageStrategy>(cfg.shortWindow, cfg.longWindow);
    }
    if (cfg.type == "breakout") {
        return std::make_unique<BreakoutStrategy>(cfg.breakoutLookback, cfg.breakoutBuffer, cfg.orderQuantity, cfg.allowShort);
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
    std::filesystem::path baseDir = absPath.parent_path();
    auto source = dataSection.value("source", std::string("csv"));
    if (source == "api") {
        cfg.dataSource = DataSourceKind::API;
        cfg.apiData = parseApiDataConfig(dataSection);
    } else {
        cfg.dataSource = DataSourceKind::CSV;
        if (!dataSection.contains("path")) {
            throw std::runtime_error("Config missing data.path");
        }
        cfg.dataPath = resolvePath(baseDir, dataSection.at("path").get<std::string>());
        if (!std::filesystem::exists(cfg.dataPath)) {
            throw std::runtime_error("CSV data file not found: " + cfg.dataPath);
        }
    }
    cfg.loaderConfig = parseLoaderConfig(dataSection);

    cfg.strategy = parseStrategyConfig(j.value("strategy", nlohmann::json::object()));
    cfg.strategies = parseStrategyList(j);
    if (cfg.strategies.empty()) {
        cfg.strategies.push_back(cfg.strategy);
    }
    ensureStrategyNames(cfg.strategies);
    cfg.strategy = cfg.strategies.front();
    const auto& engineSection = j.value("engine", nlohmann::json::object());
    cfg.initialCapital = engineSection.value("initial_capital", cfg.initialCapital);
    cfg.execution = parseExecutionConfig(engineSection.value("execution", nlohmann::json::object()));
    cfg.outputs = parseReporterOutputs(j.value("reporter", nlohmann::json::object()), baseDir);
    return cfg;
}

BacktestResult executeBacktest(const RunConfig& cfg) {
    const StrategyConfig& stratCfg = cfg.strategies.empty() ? cfg.strategy : cfg.strategies.front();
    BacktestEngine engine(cfg.execution);
    auto strategy = buildStrategy(stratCfg);
    if (cfg.dataSource == DataSourceKind::API) {
        if (!cfg.apiData) {
            throw std::runtime_error("API data source selected but no configuration provided");
        }
        APIDataLoader loader;
        const auto candles = loader.fetch(*cfg.apiData);
        return engine.run(candles, *strategy, cfg.initialCapital);
    }
    return engine.run(cfg.dataPath, cfg.loaderConfig, *strategy, cfg.initialCapital);
}

std::vector<StrategyRunResult> executeBacktests(const RunConfig& cfg) {
    std::vector<StrategyConfig> strategies = cfg.strategies;
    if (strategies.empty()) {
        strategies.push_back(cfg.strategy);
    }

    BacktestEngine engine(cfg.execution);
    std::vector<std::function<std::unique_ptr<Strategy>()>> factories;
    factories.reserve(strategies.size());
    for (const auto& stratCfg : strategies) {
        factories.emplace_back([stratCfg]() {
            return buildStrategy(stratCfg);
        });
    }

    std::vector<BacktestResult> results;
    std::vector<Candle> apiCandles;
    if (cfg.dataSource == DataSourceKind::API) {
        if (!cfg.apiData) {
            throw std::runtime_error("API data source selected but no configuration provided");
        }
        APIDataLoader loader;
        apiCandles = loader.fetch(*cfg.apiData);
        results = engine.runParallel(apiCandles, factories, cfg.initialCapital);
    } else {
        results = engine.runParallel(cfg.dataPath, cfg.loaderConfig, factories, cfg.initialCapital);
    }
    if (results.size() != strategies.size()) {
        throw std::runtime_error("Mismatch between strategy count and results");
    }

    std::vector<StrategyRunResult> runs;
    runs.reserve(strategies.size());
    for (size_t i = 0; i < strategies.size(); ++i) {
        runs.push_back({strategies[i], std::move(results[i])});
    }
    return runs;
}

ReportSummary generateReports(const RunConfig& cfg, const BacktestResult& result) {
    StrategyRunResult run;
    run.config = cfg.strategies.empty() ? cfg.strategy : cfg.strategies.front();
    run.result = result;
    auto reports = generateReports(cfg, {run});
    if (reports.empty()) {
        Reporter reporter;
        return reporter.summarize(result);
    }
    return reports.front().summary;
}

std::vector<StrategyReport> generateReports(const RunConfig& cfg, const std::vector<StrategyRunResult>& runs) {
    Reporter reporter;
    std::vector<StrategyReport> summaries;
    if (runs.empty()) {
        return summaries;
    }
    summaries.reserve(runs.size());
    for (size_t i = 0; i < runs.size(); ++i) {
        auto outputs = resolveOutputsForStrategy(cfg.outputs, runs[i].config, i, runs.size());
        auto summary = reporter.summarize(runs[i].result);

        if (outputs.jsonPath) {
            ensureParentDirectory(*outputs.jsonPath);
            reporter.writeJson(runs[i].result, *outputs.jsonPath);
        }
        if (outputs.summaryCsvPath) {
            ensureParentDirectory(*outputs.summaryCsvPath);
            reporter.writeSummaryCsv(runs[i].result, *outputs.summaryCsvPath);
        }
        if (outputs.tradesCsvPath) {
            ensureParentDirectory(*outputs.tradesCsvPath);
            reporter.writeTradesCsv(runs[i].result, *outputs.tradesCsvPath);
        }

        summaries.push_back({runs[i].config, summary, outputs});
    }
    return summaries;
}

} // namespace app
} // namespace fastquant
