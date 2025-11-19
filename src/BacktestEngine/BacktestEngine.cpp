#include "BacktestEngine.h"
#include <functional>
#include <cmath>
#include <algorithm>
#include <utility>
#include <future>
#include <stdexcept>

namespace fastquant {

BacktestResult::BacktestResult() : portfolio(0.0), initialCapital(0.0) {}
BacktestResult::BacktestResult(double initialCapital)
    : portfolio(initialCapital), initialCapital(initialCapital) {}

BacktestEngine::BacktestEngine(ExecutionConfig exec)
    : execConfig_(exec) {}

namespace {

struct PendingOrder {
    Order order;
};

double fallbackPrice(double candidate, double fallback) {
    return candidate > 0.0 ? candidate : fallback;
}

} // namespace

BacktestResult BacktestEngine::run(const std::string& csvPath, CSVDataLoader::Config cfg, Strategy& strategy, double initialCapital) {
    CSVDataLoader loader;
    std::function<void(const std::function<bool(const Candle&)>&)> streamer =
        [&](const std::function<bool(const Candle&)>& callback) {
            loader.stream(csvPath, callback, cfg);
        };
    return runWithStreamer(streamer, strategy, initialCapital);
}

BacktestResult BacktestEngine::run(const std::vector<Candle>& candles, Strategy& strategy, double initialCapital) {
    std::function<void(const std::function<bool(const Candle&)>&)> streamer =
        [&](const std::function<bool(const Candle&)>& callback) {
            for (const auto& candle : candles) {
                if (!callback(candle)) {
                    break;
                }
            }
        };
    return runWithStreamer(streamer, strategy, initialCapital);
}

BacktestResult BacktestEngine::runWithStreamer(const std::function<void(const std::function<bool(const Candle&)>&)>& streamer,
                                               Strategy& strategy,
                                               double initialCapital) {
    BacktestResult result(initialCapital);

    auto normalizeSymbol = [](const std::string& symbol) {
        return symbol.empty() ? std::string("DEFAULT") : symbol;
    };

    std::vector<PendingOrder> pendingOrders;
    size_t orderCounter = 0;

    struct EngineSink : public OrderSink {
        EngineSink(std::vector<PendingOrder>& target,
                   std::function<std::string(const std::string&)> normalizer,
                   size_t& counter)
            : pending(target), normalize(std::move(normalizer)), orderCounter(counter) {}

        void submit(const Order& o) override {
            if (o.qty <= 0.0) {
                return;
            }
            Order copy = o;
            copy.symbol = normalize(copy.symbol);
            if (copy.id.empty()) {
                copy.id = "order-" + std::to_string(orderCounter++);
            }
            pending.push_back({copy});
        }

        std::vector<PendingOrder>& pending;
        std::function<std::string(const std::string&)> normalize;
        size_t& orderCounter;
    } sink(pendingOrders, normalizeSymbol, orderCounter);

    strategy.setOrderSink(&sink);
    strategy.onStart();

    auto marketPrice = [](const Candle& c) {
        double reference = fallbackPrice(c.open, fallbackPrice(c.close, c.high));
        return reference;
    };

    auto tryFill = [&](PendingOrder& po, const Candle& candle, Trade& outTrade, double& slippageValue)->bool {
        if (po.order.symbol != normalizeSymbol(candle.symbol)) {
            return false;
        }

        double rawPrice = 0.0;
        bool canFill = false;
        double open = fallbackPrice(candle.open, candle.close);
        double high = fallbackPrice(candle.high, std::max(open, candle.close));
        double low = fallbackPrice(candle.low, std::min(open, candle.close));

        switch (po.order.type) {
            case OrderType::Market:
                rawPrice = marketPrice(candle);
                canFill = rawPrice > 0.0;
                break;
            case OrderType::Limit:
                if (po.order.side == Side::Buy) {
                    if (open <= po.order.price && open > 0.0) {
                        rawPrice = open;
                        canFill = true;
                    } else if (low <= po.order.price && po.order.price > 0.0) {
                        rawPrice = po.order.price;
                        canFill = true;
                    }
                } else {
                    if (open >= po.order.price && open > 0.0) {
                        rawPrice = open;
                        canFill = true;
                    } else if (high >= po.order.price && po.order.price > 0.0) {
                        rawPrice = po.order.price;
                        canFill = true;
                    }
                }
                break;
        }

        if (!canFill) {
            return false;
        }

        double slippageBps = std::abs(po.order.slippageBps) > 0.0 ? po.order.slippageBps : execConfig_.defaultSlippageBps;
        double slipPct = slippageBps / 10000.0;
        double finalPrice = rawPrice;
        slippageValue = 0.0;
        if (slipPct > 0.0) {
            if (po.order.side == Side::Buy) {
                finalPrice *= (1.0 + slipPct);
                slippageValue = (finalPrice - rawPrice) * po.order.qty;
            } else {
                finalPrice *= (1.0 - slipPct);
                slippageValue = (rawPrice - finalPrice) * po.order.qty;
            }
        }

        double notional = finalPrice * po.order.qty;
        double fee = execConfig_.commissionPerShare * po.order.qty
            + execConfig_.commissionBps / 10000.0 * notional;

        outTrade.orderId = po.order.id;
        outTrade.id = po.order.id + "-exec-" + std::to_string(result.trades.size() + 1);
        outTrade.side = po.order.side;
        outTrade.type = po.order.type;
        outTrade.price = finalPrice;
        outTrade.qty = po.order.qty;
        outTrade.symbol = po.order.symbol;
        outTrade.timestamp = candle.timestamp;
        outTrade.fee = fee;
        outTrade.slippageBps = slippageBps;
        return true;
    };

    auto processOrders = [&](const Candle& c) {
        auto it = pendingOrders.begin();
        while (it != pendingOrders.end()) {
            Trade trade;
            double slipValue = 0.0;
            bool filled = tryFill(*it, c, trade, slipValue);
            if (filled) {
                result.trades.push_back(trade);
                result.totalFees += trade.fee;
                result.totalSlippage += slipValue;
                ++result.ordersFilled;
                result.portfolio.applyTrade(trade);
                it = pendingOrders.erase(it);
            } else {
                bool expire = false;
                if (it->order.tif == TimeInForce::IOC || it->order.tif == TimeInForce::FOK) {
                    expire = true;
                    ++result.ordersRejected;
                }
                if (expire) {
                    it = pendingOrders.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };

    auto handleCandle = [&](const Candle& c)->bool {
        std::string sym = normalizeSymbol(c.symbol);
        result.portfolio.markPrice(sym, c.close);
        strategy.onData(c);
        processOrders(c);
        ++result.candlesProcessed;
        result.equityTimestamps.push_back(c.timestamp);
        result.equityCurve.push_back(result.portfolio.equity());
        return true;
    };

    streamer(handleCandle);

    // cancel remaining pending orders after data exhausts
    if (!pendingOrders.empty()) {
        result.ordersRejected += pendingOrders.size();
        pendingOrders.clear();
    }

    strategy.onFinish();
    strategy.setOrderSink(nullptr);

    if (result.equityCurve.empty()) {
        result.equityTimestamps.push_back(std::chrono::system_clock::now());
        result.equityCurve.push_back(result.portfolio.equity());
    }
    return result;
}

std::vector<BacktestResult> BacktestEngine::runParallel(const std::string& csvPath,
                                                        CSVDataLoader::Config cfg,
                                                        const std::vector<std::function<std::unique_ptr<Strategy>()>>& strategyFactories,
                                                        double initialCapital) {
    std::vector<BacktestResult> results;
    if (strategyFactories.empty()) {
        return results;
    }

    auto runSingle = [&](std::function<std::unique_ptr<Strategy>()> factory) {
        auto strategy = factory();
        if (!strategy) {
            throw std::runtime_error("Strategy factory returned null strategy");
        }
        BacktestEngine worker(execConfig_);
        return worker.run(csvPath, cfg, *strategy, initialCapital);
    };

    if (strategyFactories.size() == 1) {
        results.push_back(runSingle(strategyFactories.front()));
        return results;
    }

    std::vector<std::future<BacktestResult>> futures;
    futures.reserve(strategyFactories.size());
    for (const auto& factory : strategyFactories) {
        futures.emplace_back(std::async(std::launch::async, [=]() {
            return runSingle(factory);
        }));
    }

    for (auto& fut : futures) {
        results.push_back(fut.get());
    }
    return results;
}

std::vector<BacktestResult> BacktestEngine::runParallel(const std::vector<Candle>& candles,
                                                        const std::vector<std::function<std::unique_ptr<Strategy>()>>& strategyFactories,
                                                        double initialCapital) {
    std::vector<BacktestResult> results;
    if (strategyFactories.empty()) {
        return results;
    }

    auto runSingle = [&](std::function<std::unique_ptr<Strategy>()> factory) {
        auto strategy = factory();
        if (!strategy) {
            throw std::runtime_error("Strategy factory returned null strategy");
        }
        BacktestEngine worker(execConfig_);
        return worker.run(candles, *strategy, initialCapital);
    };

    if (strategyFactories.size() == 1) {
        results.push_back(runSingle(strategyFactories.front()));
        return results;
    }

    std::vector<std::future<BacktestResult>> futures;
    futures.reserve(strategyFactories.size());
    for (const auto& factory : strategyFactories) {
        futures.emplace_back(std::async(std::launch::async, [=]() {
            return runSingle(factory);
        }));
    }

    for (auto& fut : futures) {
        results.push_back(fut.get());
    }
    return results;
}

} // namespace fastquant
