#pragma once

#include "Candle.h"
#include "TimestampParser.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fastquant {

struct APIFieldMapping {
    std::string timestamp = "timestamp";
    std::string open = "open";
    std::string high = "high";
    std::string low = "low";
    std::string close = "close";
    std::string volume = "volume";
    std::string symbol = "symbol";
};

struct APIDataConfig {
    std::string endpoint;
    std::vector<std::pair<std::string, std::string>> headers;
    std::vector<std::pair<std::string, std::string>> query;
    std::optional<std::string> dataField; // optional JSON field containing the candle array
    APIFieldMapping fields;
    std::string fallbackSymbol;
};

struct APIRequest {
    std::string endpoint;
    std::vector<std::pair<std::string, std::string>> headers;
    std::vector<std::pair<std::string, std::string>> query;
};

class HttpClient {
public:
    virtual ~HttpClient() = default;
    virtual std::string get(const APIRequest& request) const = 0;
};

class CurlHttpClient : public HttpClient {
public:
    std::string get(const APIRequest& request) const override;
};

class APIDataLoader {
public:
    explicit APIDataLoader(std::unique_ptr<HttpClient> client = nullptr);

    std::vector<Candle> fetch(const APIDataConfig& cfg) const;
    void stream(const APIDataConfig& cfg, const std::function<bool(const Candle&)>& cb) const;

private:
    std::unique_ptr<HttpClient> client_;
    std::vector<Candle> parseCandles(const APIDataConfig& cfg, const std::string& payload) const;
};

} // namespace fastquant
