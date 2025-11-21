#include "../DataLoader/CSVDataLoader.h"
#include "../DataLoader/APIDataLoader.h"
#include "../BacktestEngine/BacktestEngine.h"
#include "../Strategy/MovingAverageStrategy.h"
#include "../Strategy/BreakoutStrategy.h"
#include "../Reporter/Reporter.h"
#include "../App/RunConfig.h"

// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>

using json = nlohmann::json;
using namespace fastquant;

// Global state for the session
// In a real production server, this would be per-session or in a database.
// For this local tool, a global variable is acceptable.
std::vector<Candle> g_loadedCandles;
std::mutex g_dataMutex;

int main() {
    httplib::Server svr;

    // CORS headers to allow browser fetch
    auto set_cors = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    svr.Options("/(.*)", [&](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
    });

    // Serve static files from frontend directory
    // We attempt to mount both relative paths to cover running from root or build/
    svr.set_mount_point("/", "./frontend");
    svr.set_mount_point("/", "../frontend");

    // Redirect root to index.html
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_redirect("/index.html");
    });

    // Endpoint: /list-examples
    svr.Get("/list-examples", [&](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        // Hardcoded list for now since we can't easily scan directories cross-platform without std::filesystem (C++17)
        // or platform specific code. Assuming standard examples folder structure.
        std::vector<std::string> files = {
            "examples/sample_prices.csv",
            "examples/sample_noheader.csv"
        };
        json response = files;
        res.set_content(response.dump(), "application/json");
    });

    // Endpoint: /load-data
    svr.Post("/load-data", [&](const httplib::Request& req, httplib::Response& res) {
        set_cors(res);
        try {
            auto j = json::parse(req.body);
            std::string source = j.value("source", "csv");
            std::vector<Candle> newCandles;

            if (source == "api") {
                std::string symbol = j.value("symbol", "BTCUSDT");
                std::string interval = j.value("interval", "1h");
                
                APIDataConfig cfg;
                cfg.endpoint = "https://api.binance.com/api/v3/klines";
                cfg.query = {
                    {"symbol", symbol},
                    {"interval", interval},
                    {"limit", "500"}
                };
                
                // Binance returns [ [time, open, high, low, close, vol, ...], ... ]
                // Map indices to fields
                cfg.fields.timestamp = "0";
                cfg.fields.open = "1";
                cfg.fields.high = "2";
                cfg.fields.low = "3";
                cfg.fields.close = "4";
                cfg.fields.volume = "5";
                cfg.fields.symbol = ""; 
                cfg.fallbackSymbol = symbol;

                APIDataLoader loader(std::make_unique<CurlHttpClient>());
                newCandles = loader.fetch(cfg);
            } else {
                std::string path = j.value("path", "");
                if (path.empty()) {
                    throw std::runtime_error("Missing 'path' in request body");
                }
                CSVDataLoader loader;
                newCandles = loader.load(path);
            }

            // Load all data into memory for the server session
            std::lock_guard<std::mutex> lock(g_dataMutex);
            g_loadedCandles = newCandles; 

            json response;
            response["rows"] = g_loadedCandles.size();
            if (!g_loadedCandles.empty()) {
                // Simple timestamp formatting or just raw count
                // Assuming Candle has a timestamp field which is a time_point
                // We can just return the count and maybe the symbol of the first candle
                response["symbol"] = g_loadedCandles.front().symbol;
                // TODO: Format dates if needed, for now just returning basic info
            }
            
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Endpoint: /run-backtest
    svr.Post("/run-backtest", [&](const httplib::Request& req, httplib::Response& res) {
        set_cors(res);
        try {
            auto j = json::parse(req.body);
            std::vector<Candle> candles;
            {
                std::lock_guard<std::mutex> lock(g_dataMutex);
                if (g_loadedCandles.empty()) {
                    throw std::runtime_error("No data loaded. Please call /load-data first.");
                }
                candles = g_loadedCandles; // Copy for thread safety during run if we were async
            }

            // Setup Strategy
            std::unique_ptr<Strategy> strategy;
            std::string strategyType = j.value("strategyType", "MA");

            if (strategyType == "Breakout") {
                int lookback = j.value("lookback", 20);
                double threshold = j.value("threshold", 0.0);
                strategy = std::make_unique<BreakoutStrategy>(lookback, threshold);
            } else {
                int period = j.value("period", 20);
                int longWindow = period;
                int shortWindow = std::max(1, period / 4);
                strategy = std::make_unique<MovingAverageStrategy>(shortWindow, longWindow);
            }
            
            // Setup Engine
            BacktestEngine engine;
            // Configure engine (simplified)
            ExecutionConfig execCfg; 
            execCfg.defaultSlippageBps = 5; // Example default
            engine.setExecutionConfig(execCfg);

            // Run
            BacktestResult result = engine.run(candles, *strategy);
            
            json response;
            response["strategy"] = strategyType;
            response["trades"] = result.trades.size();
            
            // Calculate win rate (Simplified: we don't track round-trip trades yet)
            // int wins = 0;
            // for (const auto& t : result.trades) {
            //     if (t.realizedPnL > 0) wins++;
            // }
            // response["winRate"] = result.trades.empty() ? 0.0 : (double)wins / result.trades.size();
            response["winRate"] = 0.0; 
            
            // Total Profit (Realized + Unrealized usually, but let's use realized for simplicity or portfolio equity)
            // BacktestResult has equityCurve, let's use the final equity - initial capital
            if (!result.equityCurve.empty()) {
                response["totalProfit"] = result.equityCurve.back() - result.initialCapital;
            } else {
                response["totalProfit"] = 0.0;
            }

            // Max Drawdown calculation (simplified)
            double maxPeak = -1e9;
            double maxDD = 0.0;
            for (double equity : result.equityCurve) {
                if (equity > maxPeak) maxPeak = equity;
                double dd = (maxPeak - equity) / maxPeak;
                if (dd > maxDD) maxDD = dd;
            }
            response["maxDrawdown"] = maxDD;

            // Equity Curve for Chart
            std::vector<json> equityData;
            for (size_t i = 0; i < result.equityCurve.size(); ++i) {
                long long ts = 0;
                if (i < result.equityTimestamps.size()) {
                    ts = std::chrono::duration_cast<std::chrono::seconds>(
                        result.equityTimestamps[i].time_since_epoch()
                    ).count();
                }
                equityData.push_back({{"time", ts}, {"value", result.equityCurve[i]}});
            }
            response["equityCurve"] = equityData;

            // Recent Trades List (Last 50)
            std::vector<json> recentTrades;
            size_t tradeCount = result.trades.size();
            size_t startIdx = tradeCount > 50 ? tradeCount - 50 : 0;
            
            for (size_t i = startIdx; i < tradeCount; ++i) {
                const auto& t = result.trades[i];
                long long ts = std::chrono::duration_cast<std::chrono::seconds>(
                    t.timestamp.time_since_epoch()
                ).count();
                
                recentTrades.push_back({
                    {"date", ts},
                    {"type", t.side == Side::Buy ? "BUY" : "SELL"},
                    {"price", t.price},
                    {"qty", t.qty},
                    {"pnl", 0.0} // Individual trade PnL not tracked in Trade struct yet
                });
            }
            // Reverse to show newest first
            std::reverse(recentTrades.begin(), recentTrades.end());
            response["recentTrades"] = recentTrades;

            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    std::cout << "Server started at http://localhost:8080" << std::endl;
    svr.listen("localhost", 8080);

    return 0;
}
