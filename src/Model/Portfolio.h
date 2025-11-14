#pragma once

#include <unordered_map>
#include <string>
#include "Trade.h"

namespace fastquant {

struct Position {
    std::string symbol;
    double qty{0.0}; // signed: >0 long, <0 short
    double avgPrice{0.0};

    double marketValue(double markPrice) const {
        return qty * markPrice;
    }

    double unrealized(double markPrice) const {
        return qty * (markPrice - avgPrice);
    }
};

class Portfolio {
public:
    explicit Portfolio(double initialCash = 100000.0);

    double cash() const { return cash_; }
    double realizedPnl() const { return realizedPnl_; }

    void applyTrade(const Trade& trade);
    void markPrice(const std::string& symbol, double price);

    double positionValue() const;
    double unrealizedPnl() const;
    double equity() const { return cash_ + positionValue(); }

    const std::unordered_map<std::string, Position>& positions() const { return positions_; }
    double lastPrice(const std::string& symbol) const;

private:
    static std::string normalizeSymbol(const std::string& symbol);

    double cash_;
    double realizedPnl_{0.0};
    std::unordered_map<std::string, Position> positions_;
    std::unordered_map<std::string, double> marks_;
};

} // namespace fastquant
