#include "../App/RunConfig.h"
#include "../App/EnvLoader.h"
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using fastquant::BacktestResult;
using fastquant::ReportSummary;
using fastquant::app::StrategyConfig;
using fastquant::app::StrategyReport;

namespace {

constexpr const char* kVersion = "0.2.0";

struct CliOptions {
    std::string configPath = "fastquant.config.json";
    bool showSummary = true;
    bool validateOnly = false;
    bool printResolved = false;
};

void printUsage() {
    std::cout << "Usage: fastquant_cli [options] [config.json]\n";
    std::cout << "Options:\n"
              << "  -c, --config <path>   Override config path (defaults to fastquant.config.json)\n"
              << "      --no-summary      Skip printing summary to stdout\n"
              << "      --validate        Only validate the config and exit\n"
              << "      --print-config    Print resolved config details after loading\n"
              << "      --version         Print CLI version\n"
              << "  -h, --help           Show this help text\n";
}

void printSummary(const ReportSummary& summary, const fastquant::BacktestResult& result, const std::string& label) {
    std::cout << "\n=== FastQuant Report";
    if (!label.empty()) {
        std::cout << " - " << label;
    }
    std::cout << " ===\n";
    std::cout << "Initial capital : " << summary.initialCapital << '\n';
    std::cout << "Final equity    : " << summary.finalEquity << '\n';
    std::cout << "Total return    : " << std::fixed << std::setprecision(2)
              << (summary.totalReturn * 100.0) << "%\n";
    std::cout << "Realized PnL    : " << summary.realizedPnl << '\n';
    std::cout << "Unrealized PnL  : " << summary.unrealizedPnl << '\n';
    std::cout << "Max drawdown    : " << summary.maxDrawdown << '\n';
    std::cout << "Trades          : " << summary.trades
              << " (" << summary.winningTrades << " winners / "
              << summary.losingTrades << " losers, win rate "
              << std::setprecision(2) << std::fixed << (summary.winRate * 100.0)
              << "%)\n";
    std::cout << "Total fees       : " << summary.totalFees << '\n';
    std::cout << "Total slippage   : " << summary.totalSlippage << '\n';
    std::cout << "Orders filled    : " << summary.ordersFilled
              << ", rejected: " << summary.ordersRejected << '\n';
    std::cout << "Candles processed: " << result.candlesProcessed << "\n";
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions opts;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage();
            std::exit(0);
        } else if (arg == "--version") {
            std::cout << "fastquant_cli " << kVersion << '\n';
            std::exit(0);
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value for --config");
            }
            opts.configPath = argv[++i];
        } else if (arg == "--no-summary") {
            opts.showSummary = false;
        } else if (arg == "--validate") {
            opts.validateOnly = true;
        } else if (arg == "--print-config") {
            opts.printResolved = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::ostringstream oss;
            oss << "Unknown option: " << arg;
            throw std::runtime_error(oss.str());
        } else {
            positional.push_back(arg);
        }
    }
    if (!positional.empty()) {
        opts.configPath = positional.back();
    }
    return opts;
}

void describeStrategy(const StrategyConfig& strat) {
    std::cout << "    - " << strat.name << " [" << strat.type << "]";
    if (strat.type == "moving_average") {
        std::cout << " short=" << strat.shortWindow << ", long=" << strat.longWindow;
    } else if (strat.type == "breakout") {
        std::cout << " lookback=" << strat.breakoutLookback
                  << ", buffer=" << strat.breakoutBuffer
                  << ", qty=" << strat.orderQuantity
                  << ", allow_short=" << (strat.allowShort ? "true" : "false");
    }
    std::cout << '\n';
}

void printResolved(const fastquant::app::RunConfig& cfg) {
    std::cout << "\nResolved config:\n";
    std::cout << "  Source file   : " << cfg.sourceConfigPath << '\n';
    std::cout << "  Data source   : "
              << (cfg.dataSource == fastquant::app::DataSourceKind::API ? "API" : "CSV") << '\n';
    if (cfg.dataSource == fastquant::app::DataSourceKind::CSV) {
        std::cout << "  Data path     : " << cfg.dataPath << '\n';
    } else if (cfg.apiData) {
        std::cout << "  Endpoint      : " << cfg.apiData->endpoint << '\n';
        std::cout << "  Data field    : "
                  << (cfg.apiData->dataField ? *cfg.apiData->dataField : std::string("-")) << '\n';
        std::cout << "  Field map     : ts=" << cfg.apiData->fields.timestamp
                  << ", open=" << cfg.apiData->fields.open
                  << ", high=" << cfg.apiData->fields.high
                  << ", low=" << cfg.apiData->fields.low
                  << ", close=" << cfg.apiData->fields.close
                  << ", volume=" << cfg.apiData->fields.volume
                  << ", symbol=" << (cfg.apiData->fields.symbol.empty() ? "<fallback>" : cfg.apiData->fields.symbol)
                  << '\n';
    }
    std::cout << "  Initial cap   : " << cfg.initialCapital << '\n';
    std::cout << "  Strategies    : " << cfg.strategies.size() << '\n';
    for (const auto& strat : cfg.strategies) {
        describeStrategy(strat);
    }
    std::cout << "  Execution     : slippage=" << cfg.execution.defaultSlippageBps
              << " bps, per-share fee=" << cfg.execution.commissionPerShare
              << ", bps fee=" << cfg.execution.commissionBps << '\n';
    std::cout << "  Reporter paths: JSON="
              << (cfg.outputs.jsonPath ? *cfg.outputs.jsonPath : "-")
              << ", summary="
              << (cfg.outputs.summaryCsvPath ? *cfg.outputs.summaryCsvPath : "-")
              << ", trades="
              << (cfg.outputs.tradesCsvPath ? *cfg.outputs.tradesCsvPath : "-")
              << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        auto opts = parseArgs(argc, argv);
        fastquant::app::loadEnvFile(".env");
        fastquant::app::loadEnvFile(".env.local");
        std::filesystem::path cfgPathCandidate = opts.configPath;
        if (cfgPathCandidate.has_parent_path()) {
            fastquant::app::loadEnvFile(cfgPathCandidate.parent_path() / ".env");
        }
        auto cfg = fastquant::app::loadRunConfig(opts.configPath);

        if (opts.printResolved) {
            printResolved(cfg);
        }
        if (opts.validateOnly) {
            std::cout << "Config validated successfully." << std::endl;
            return 0;
        }

        auto runs = fastquant::app::executeBacktests(cfg);
        auto reports = fastquant::app::generateReports(cfg, runs);

        if (cfg.outputs.printSummary && opts.showSummary) {
            for (size_t i = 0; i < reports.size(); ++i) {
                printSummary(reports[i].summary, runs[i].result, reports[i].config.name);
            }
        }

        bool wroteArtifacts = false;
        for (const auto& report : reports) {
            const auto& label = report.config.name;
            const auto& outputs = report.outputs;
            if (outputs.jsonPath) {
                std::cout << "[" << label << "] JSON report written to    : " << *outputs.jsonPath << '\n';
                wroteArtifacts = true;
            }
            if (outputs.summaryCsvPath) {
                std::cout << "[" << label << "] Summary CSV written to    : " << *outputs.summaryCsvPath << '\n';
                wroteArtifacts = true;
            }
            if (outputs.tradesCsvPath) {
                std::cout << "[" << label << "] Trades CSV written to     : " << *outputs.tradesCsvPath << '\n';
                wroteArtifacts = true;
            }
        }

        if (!wroteArtifacts) {
            std::cout << "No report files configured; summary printed to stdout only." << std::endl;
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}
