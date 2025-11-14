#include "BacktestEngine.h"

namespace fastquant {

BacktestResult::BacktestResult() : portfolio(0.0), initialCapital(0.0) {}
BacktestResult::BacktestResult(double initialCapital)
    : portfolio(initialCapital), initialCapital(initialCapital) {}

BacktestResult BacktestEngine::run(const std::string& csvPath, CSVDataLoader::Config cfg, Strategy& strategy, double initialCapital) {
    CSVDataLoader loader;
    BacktestResult result(initialCapital);

    auto normalizeSymbol = [](const std::string& symbol) {
        return symbol.empty() ? std::string("DEFAULT") : symbol;
    };

    // Order sink implementation that converts orders into immediate trades at order.price.
    struct EngineSink : public OrderSink {
        EngineSink(std::vector<Trade>& t, Portfolio& p) : trades(t), portfolio(p) {}
        void submit(const Order& o) override {
            Trade tr;
            tr.orderId = o.id;
            tr.id = o.id + "-exec";
            tr.side = o.side;
            tr.price = o.price;
            tr.qty = o.qty;
            tr.symbol = o.symbol;
            tr.timestamp = o.timestamp;
            trades.push_back(tr);
            portfolio.applyTrade(tr);
        }
        std::vector<Trade>& trades;
        Portfolio& portfolio;
    } sink(result.trades, result.portfolio);

    strategy.setOrderSink(&sink);

    strategy.onStart();

    // Stream candles to strategy. Early-stop supported if strategy needs it in future.
    loader.stream(csvPath, [&](const Candle& c)->bool {
        std::string sym = normalizeSymbol(c.symbol);
        result.portfolio.markPrice(sym, c.close);
        strategy.onData(c);
        ++result.candlesProcessed;
        result.equityTimestamps.push_back(c.timestamp);
        result.equityCurve.push_back(result.portfolio.equity());
        return true; // continue
    }, cfg);

    strategy.onFinish();
    // detach sink
    strategy.setOrderSink(nullptr);
    if (result.equityCurve.empty()) {
        result.equityTimestamps.push_back(std::chrono::system_clock::now());
        result.equityCurve.push_back(result.portfolio.equity());
    }
    return result;
}

} // namespace fastquant
