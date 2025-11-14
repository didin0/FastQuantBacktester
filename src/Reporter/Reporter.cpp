#include "Reporter.h"
#include "../Model/Portfolio.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace fastquant {

namespace {
constexpr double EPS = 1e-9;
}

std::string Reporter::formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

ReportSummary Reporter::summarize(const BacktestResult& result) const {
    ReportSummary summary;
    summary.initialCapital = result.initialCapital;
    summary.finalEquity = result.portfolio.equity();
    summary.realizedPnl = result.portfolio.realizedPnl();
    summary.unrealizedPnl = result.portfolio.unrealizedPnl();
    summary.totalReturn = (summary.initialCapital > 0.0)
        ? ((summary.finalEquity / summary.initialCapital) - 1.0)
        : 0.0;
    summary.trades = result.trades.size();

    // compute drawdown from equity curve
    double peak = summary.initialCapital;
    double trough = summary.initialCapital;
    double maxDd = 0.0;
    if (!result.equityCurve.empty()) {
        peak = result.equityCurve.front();
        trough = peak;
        for (double equity : result.equityCurve) {
            if (equity > peak) {
                peak = equity;
                trough = equity;
            }
            if (equity < trough) {
                trough = equity;
                double dd = (peak - trough);
                if (dd > maxDd) {
                    maxDd = dd;
                }
            }
        }
    }
    summary.peakEquity = peak;
    summary.troughEquity = trough;
    summary.maxDrawdown = maxDd;

    // win/loss counts by replaying realized pnl deltas
    Portfolio replay(result.initialCapital);
    double prevRealized = 0.0;
    for (const auto& tr : result.trades) {
        replay.applyTrade(tr);
        double current = replay.realizedPnl();
        double delta = current - prevRealized;
        if (delta > EPS) {
            summary.winningTrades++;
        } else if (delta < -EPS) {
            summary.losingTrades++;
        }
        prevRealized = current;
    }
    if (summary.winningTrades + summary.losingTrades > 0) {
        summary.winRate = static_cast<double>(summary.winningTrades)
            / static_cast<double>(summary.winningTrades + summary.losingTrades);
    }

    return summary;
}

void Reporter::writeJson(const BacktestResult& result, const std::string& path) const {
    auto summary = summarize(result);
    nlohmann::json j;
    j["summary"] = {
        {"initial_capital", summary.initialCapital},
        {"final_equity", summary.finalEquity},
        {"total_return", summary.totalReturn},
        {"realized_pnl", summary.realizedPnl},
        {"unrealized_pnl", summary.unrealizedPnl},
        {"max_drawdown", summary.maxDrawdown},
        {"peak_equity", summary.peakEquity},
        {"trough_equity", summary.troughEquity},
        {"trades", summary.trades},
        {"winning_trades", summary.winningTrades},
        {"losing_trades", summary.losingTrades},
        {"win_rate", summary.winRate}
    };

    nlohmann::json tradesJson = nlohmann::json::array();
    for (const auto& tr : result.trades) {
        tradesJson.push_back({
            {"id", tr.id},
            {"order_id", tr.orderId},
            {"side", tr.side == Side::Buy ? "BUY" : "SELL"},
            {"price", tr.price},
            {"qty", tr.qty},
            {"symbol", tr.symbol},
            {"timestamp", formatTimestamp(tr.timestamp)}
        });
    }
    j["trades"] = tradesJson;

    nlohmann::json equityJson = nlohmann::json::array();
    for (size_t i = 0; i < result.equityCurve.size(); ++i) {
        nlohmann::json point;
        point["equity"] = result.equityCurve[i];
        if (i < result.equityTimestamps.size()) {
            point["timestamp"] = formatTimestamp(result.equityTimestamps[i]);
        }
        equityJson.push_back(point);
    }
    j["equity_curve"] = equityJson;

    std::ofstream ofs(path);
    ofs << std::setw(2) << j;
}

void Reporter::writeSummaryCsv(const BacktestResult& result, const std::string& path) const {
    auto summary = summarize(result);
    std::ofstream ofs(path);
    ofs << "initial_capital,final_equity,total_return,realized_pnl,unrealized_pnl,max_drawdown,win_rate,trades,winning_trades,losing_trades\n";
    ofs << summary.initialCapital << ','
        << summary.finalEquity << ','
        << summary.totalReturn << ','
        << summary.realizedPnl << ','
        << summary.unrealizedPnl << ','
        << summary.maxDrawdown << ','
        << summary.winRate << ','
        << summary.trades << ','
        << summary.winningTrades << ','
        << summary.losingTrades << '\n';
}

void Reporter::writeTradesCsv(const BacktestResult& result, const std::string& path) const {
    std::ofstream ofs(path);
    ofs << "trade_id,order_id,side,price,qty,symbol,timestamp\n";
    for (const auto& tr : result.trades) {
        ofs << tr.id << ','
            << tr.orderId << ','
            << (tr.side == Side::Buy ? "BUY" : "SELL") << ','
            << tr.price << ','
            << tr.qty << ','
            << tr.symbol << ','
            << formatTimestamp(tr.timestamp) << '\n';
    }
}

} // namespace fastquant
