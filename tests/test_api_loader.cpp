#include <catch2/catch.hpp>

#include "../src/DataLoader/APIDataLoader.h"

using namespace fastquant;

namespace {

class StubHttpClient : public HttpClient {
public:
    explicit StubHttpClient(std::string body) : body_(std::move(body)) {}
    std::string get(const APIRequest&) const override { return body_; }

private:
    std::string body_;
};

} // namespace

TEST_CASE("APIDataLoader parses top-level array responses") {
    std::string json = R"([
        {"timestamp":"2024-01-01T00:00:00Z","open":100,"high":105,"low":95,"close":102,"volume":1200,"symbol":"TEST"},
        {"timestamp":"2024-01-02T00:00:00Z","open":102,"high":106,"low":101,"close":105,"volume":1500}
    ])";

    APIDataConfig cfg;
    cfg.endpoint = "https://example.com/bars";
    cfg.fallbackSymbol = "TEST";

    APIDataLoader loader(std::make_unique<StubHttpClient>(json));
    auto candles = loader.fetch(cfg);
    REQUIRE(candles.size() == 2);
    REQUIRE(candles[0].close == Approx(102));
    REQUIRE(candles[1].symbol == "TEST");
}

TEST_CASE("APIDataLoader handles nested data field and custom mappings") {
    std::string json = R"({
        "data": [
            {"ts":"2024-01-01T00:00:00Z","o":10,"h":12,"l":9,"c":11,"v":1000,"ticker":"ABC"}
        ]
    })";

    APIDataConfig cfg;
    cfg.endpoint = "https://example.com/custom";
    cfg.dataField = std::string("data");
    cfg.fields.timestamp = "ts";
    cfg.fields.open = "o";
    cfg.fields.high = "h";
    cfg.fields.low = "l";
    cfg.fields.close = "c";
    cfg.fields.volume = "v";
    cfg.fields.symbol = "ticker";

    APIDataLoader loader(std::make_unique<StubHttpClient>(json));
    auto candles = loader.fetch(cfg);
    REQUIRE(candles.size() == 1);
    REQUIRE(candles[0].symbol == "ABC");
    REQUIRE(candles[0].volume == Approx(1000));
}

TEST_CASE("APIDataLoader parses Binance-style array candles") {
    std::string json = R"([
        [1700000000000,"100.0","110.0","90.0","105.0","2500"],
        [1700003600000,"105.0","115.0","101.0","112.0","3000"]
    ])";

    APIDataConfig cfg;
    cfg.endpoint = "https://api.binance.com/api/v3/klines";
    cfg.fallbackSymbol = "BTCUSDT";
    cfg.fields.timestamp = "0";
    cfg.fields.open = "1";
    cfg.fields.high = "2";
    cfg.fields.low = "3";
    cfg.fields.close = "4";
    cfg.fields.volume = "5";
    cfg.fields.symbol = ""; // fall back to config symbol

    APIDataLoader loader(std::make_unique<StubHttpClient>(json));
    auto candles = loader.fetch(cfg);
    REQUIRE(candles.size() == 2);
    REQUIRE(candles[0].open == Approx(100.0));
    REQUIRE(candles[0].symbol == "BTCUSDT");
    REQUIRE(candles[1].close == Approx(112.0));
}
