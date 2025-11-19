#include "Portfolio.h"
#include <cmath>
#include <algorithm>

namespace fastquant {

namespace {
constexpr double EPS = 1e-9;
}

Portfolio::Portfolio(double initialCash) : cash_(initialCash) {}

std::string Portfolio::normalizeSymbol(const std::string& symbol) {
    if (!symbol.empty()) return symbol;
    return "DEFAULT";
}

void Portfolio::applyTrade(const Trade& trade) {
    std::string sym = normalizeSymbol(trade.symbol);
    Position& pos = positions_[sym];
    pos.symbol = sym;

    double signedQty = (trade.side == Side::Buy ? trade.qty : -trade.qty);
    if (std::abs(signedQty) < EPS) return;

    // cash decreases on buys, increases on sells
    cash_ -= signedQty * trade.price;
    cash_ -= trade.fee;

    marks_[sym] = trade.price;

    double prevQty = pos.qty;
    if (std::abs(prevQty) < EPS) {
        pos.qty = signedQty;
        pos.avgPrice = trade.price;
        return;
    }

    bool sameDirection = (prevQty > 0 && signedQty > 0) || (prevQty < 0 && signedQty < 0);
    if (sameDirection) {
        double totalAbs = std::abs(prevQty) + std::abs(signedQty);
        pos.avgPrice = (pos.avgPrice * std::abs(prevQty) + trade.price * std::abs(signedQty)) / totalAbs;
        pos.qty = prevQty + signedQty;
        return;
    }

    double closingQty = std::min(std::abs(prevQty), std::abs(signedQty));
    if (prevQty > 0) {
        realizedPnl_ += (trade.price - pos.avgPrice) * closingQty;
    } else {
        realizedPnl_ += (pos.avgPrice - trade.price) * closingQty;
    }

    double newQty = prevQty + signedQty;
    if (std::abs(newQty) < EPS) {
        pos.qty = 0.0;
        pos.avgPrice = 0.0;
    } else if ((prevQty > 0 && newQty > 0) || (prevQty < 0 && newQty < 0)) {
        // partial close, keep avg price
        pos.qty = newQty;
    } else {
        // position flipped direction; remaining qty entered at trade price
        pos.qty = newQty;
        pos.avgPrice = trade.price;
    }
}

void Portfolio::markPrice(const std::string& symbol, double price) {
    if (price <= 0.0) return;
    std::string sym = normalizeSymbol(symbol);
    marks_[sym] = price;
}

double Portfolio::lastPrice(const std::string& symbol) const {
    std::string sym = normalizeSymbol(symbol);
    auto it = marks_.find(sym);
    if (it != marks_.end()) return it->second;
    auto posIt = positions_.find(sym);
    if (posIt != positions_.end()) return posIt->second.avgPrice;
    return 0.0;
}

double Portfolio::positionValue() const {
    double value = 0.0;
    for (const auto& [sym, pos] : positions_) {
        if (std::abs(pos.qty) < EPS) continue;
        double mark = lastPrice(sym);
        value += pos.marketValue(mark);
    }
    return value;
}

double Portfolio::unrealizedPnl() const {
    double u = 0.0;
    for (const auto& [sym, pos] : positions_) {
        if (std::abs(pos.qty) < EPS) continue;
        double mark = lastPrice(sym);
        u += pos.unrealized(mark);
    }
    return u;
}

} // namespace fastquant
