#include "../App/RunConfig.h"
#include <iomanip>
#include <iostream>
#include <string>

using fastquant::BacktestResult;
using fastquant::ReportSummary;

namespace {

void printUsage() {
    std::cout << "Usage: fastquant_cli <config.json>" << std::endl;
    std::cout << "       Defaults to fastquant.config.json in the current directory." << std::endl;
}

void printSummary(const ReportSummary& summary, const fastquant::BacktestResult& result) {
    std::cout << "\n=== FastQuant Report ===\n";
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
    std::cout << "Candles processed: " << result.candlesProcessed << "\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 1) {
            std::string arg1 = argv[1];
            if (arg1 == "-h" || arg1 == "--help") {
                printUsage();
                return 0;
            }
        }

        std::string configPath = (argc > 1) ? argv[1] : "fastquant.config.json";
        auto cfg = fastquant::app::loadRunConfig(configPath);
        auto result = fastquant::app::executeBacktest(cfg);
        auto summary = fastquant::app::generateReports(cfg, result);

        if (cfg.outputs.printSummary) {
            printSummary(summary, result);
        }

        if (cfg.outputs.jsonPath) {
            std::cout << "JSON report written to    : " << *cfg.outputs.jsonPath << '\n';
        }
        if (cfg.outputs.summaryCsvPath) {
            std::cout << "Summary CSV written to    : " << *cfg.outputs.summaryCsvPath << '\n';
        }
        if (cfg.outputs.tradesCsvPath) {
            std::cout << "Trades CSV written to     : " << *cfg.outputs.tradesCsvPath << '\n';
        }

        if (!cfg.outputs.jsonPath && !cfg.outputs.summaryCsvPath && !cfg.outputs.tradesCsvPath) {
            std::cout << "No report files configured; summary printed to stdout only." << std::endl;
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}
