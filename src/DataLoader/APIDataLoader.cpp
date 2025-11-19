#include "APIDataLoader.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace fastquant {
namespace {

size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), realSize);
    return realSize;
}

std::string buildUrl(CURL* curl, const APIRequest& request) {
    if (request.query.empty()) {
        return request.endpoint;
    }

    std::string url = request.endpoint;
    bool hasQuery = url.find('?') != std::string::npos;
    url += hasQuery ? '&' : '?';

    bool first = true;
    for (const auto& kv : request.query) {
        if (!first) {
            url += '&';
        }
        first = false;
        char* key = curl_easy_escape(curl, kv.first.c_str(), static_cast<int>(kv.first.size()));
        char* value = curl_easy_escape(curl, kv.second.c_str(), static_cast<int>(kv.second.size()));
        if (!key || !value) {
            if (key) curl_free(key);
            if (value) curl_free(value);
            throw std::runtime_error("Failed to escape query parameter");
        }
        url += key;
        url += '=';
        url += value;
        curl_free(key);
        curl_free(value);
    }
    return url;
}

const nlohmann::json* resolveField(const nlohmann::json& entry, const std::string& field) {
    if (field.empty()) {
        return nullptr;
    }
    if (entry.is_object()) {
        auto it = entry.find(field);
        if (it == entry.end()) {
            return nullptr;
        }
        return &(*it);
    }
    if (entry.is_array()) {
        size_t idx = 0;
        try {
            idx = static_cast<size_t>(std::stoull(field));
        } catch (const std::exception&) {
            throw std::runtime_error("Field '" + field + "' must be numeric when parsing array-based API payloads");
        }
        if (idx >= entry.size()) {
            return nullptr;
        }
        return &entry[idx];
    }
    return nullptr;
}

std::optional<std::string> getStringField(const nlohmann::json& entry, const std::string& field) {
    const auto* node = resolveField(entry, field);
    if (!node || node->is_null()) {
        return std::nullopt;
    }
    if (node->is_string()) {
        return node->get<std::string>();
    }
    if (node->is_number_integer()) {
        return std::to_string(node->get<long long>());
    }
    if (node->is_number_unsigned()) {
        return std::to_string(node->get<unsigned long long>());
    }
    if (node->is_number_float()) {
        double value = node->get<double>();
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
        auto str = oss.str();
        if (auto pos = str.find('.'); pos != std::string::npos) {
            while (!str.empty() && str.back() == '0') {
                str.pop_back();
            }
            if (!str.empty() && str.back() == '.') {
                str.pop_back();
            }
        }
        if (str.empty()) {
            str = "0";
        }
        return str;
    }
    return node->dump();
}

double getNumericField(const nlohmann::json& entry, const std::string& field) {
    const auto* node = resolveField(entry, field);
    if (!node || node->is_null()) {
        std::ostringstream oss;
        oss << "Missing numeric field '" << field << "' in API response";
        throw std::runtime_error(oss.str());
    }
    if (node->is_number_float()) {
        return node->get<double>();
    }
    if (node->is_number_integer()) {
        return static_cast<double>(node->get<long long>());
    }
    if (node->is_number_unsigned()) {
        return static_cast<double>(node->get<unsigned long long>());
    }
    if (node->is_string()) {
        try {
            return std::stod(node->get<std::string>());
        } catch (const std::exception& ex) {
            std::ostringstream oss;
            oss << "Unable to convert field '" << field << "' to double: " << ex.what();
            throw std::runtime_error(oss.str());
        }
    }
    std::ostringstream oss;
    oss << "Unsupported type for numeric field '" << field << "'";
    throw std::runtime_error(oss.str());
}

std::string getSymbolField(const nlohmann::json& entry, const APIDataConfig& cfg) {
    if (!cfg.fields.symbol.empty()) {
        if (auto value = getStringField(entry, cfg.fields.symbol)) {
            return *value;
        }
    }
    return cfg.fallbackSymbol;
}

void logTimestampFailure(const std::string& field,
                         const nlohmann::json* node,
                         const std::string& reason) {
    static int logged = 0;
    if (logged >= 3) {
        return;
    }
    ++logged;
    if (node) {
        spdlog::warn("Timestamp parse issue ({}): field={}, sample={}",
                     reason,
                     field,
                     node->dump());
    } else {
        spdlog::warn("Timestamp parse issue ({}): field={}, sample=<null>", reason, field);
    }
}

bool assignTimestampFromIntegral(long long value, std::chrono::system_clock::time_point& out) {
    if (std::llabs(value) > 100000000000LL) {
        out = std::chrono::system_clock::time_point(std::chrono::milliseconds(value));
    } else {
        out = std::chrono::system_clock::from_time_t(static_cast<time_t>(value));
    }
    return true;
}

bool parseTimestampField(const nlohmann::json& entry,
                         const std::string& field,
                         std::chrono::system_clock::time_point& out) {
    const auto* node = resolveField(entry, field);
    if (!node || node->is_null()) {
        logTimestampFailure(field, node, "missing field");
        return false;
    }
    try {
        if (node->is_string()) {
            return parseTimestamp(node->get<std::string>(), out);
        }
        if (node->is_number_integer()) {
            return assignTimestampFromIntegral(node->get<long long>(), out);
        }
        if (node->is_number_unsigned()) {
            auto value = node->get<unsigned long long>();
            if (value > static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
                out = std::chrono::system_clock::time_point(std::chrono::milliseconds(value));
                return true;
            }
            return assignTimestampFromIntegral(static_cast<long long>(value), out);
        }
        if (node->is_number_float()) {
            double value = node->get<double>();
            long double integral;
            std::modf(value, &integral);
            return assignTimestampFromIntegral(static_cast<long long>(integral), out);
        }
    } catch (const std::exception& ex) {
        logTimestampFailure(field, node, ex.what());
        return false;
    }
    logTimestampFailure(field, node, "unsupported type");
    return false;
}

} // namespace

std::string CurlHttpClient::get(const APIRequest& request) const {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string response;
    struct curl_slist* headers = nullptr;

    try {
        std::string url = buildUrl(curl, request);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        for (const auto& header : request.headers) {
            std::string line = header.first + ": " + header.second;
            headers = curl_slist_append(headers, line.c_str());
        }
        if (headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
        }

        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode >= 400) {
            std::ostringstream oss;
            oss << "HTTP error " << httpCode << " while fetching " << request.endpoint;
            throw std::runtime_error(oss.str());
        }
    } catch (...) {
        if (headers) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);
        throw;
    }

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return response;
}

APIDataLoader::APIDataLoader(std::unique_ptr<HttpClient> client)
    : client_(client ? std::move(client) : std::make_unique<CurlHttpClient>()) {}

std::vector<Candle> APIDataLoader::fetch(const APIDataConfig& cfg) const {
    APIRequest request{cfg.endpoint, cfg.headers, cfg.query};
    auto payload = client_->get(request);
    return parseCandles(cfg, payload);
}

void APIDataLoader::stream(const APIDataConfig& cfg, const std::function<bool(const Candle&)>& cb) const {
    auto candles = fetch(cfg);
    for (const auto& candle : candles) {
        if (!cb(candle)) {
            break;
        }
    }
}

std::vector<Candle> APIDataLoader::parseCandles(const APIDataConfig& cfg, const std::string& payload) const {
    std::vector<Candle> candles;
    if (payload.empty()) {
        return candles;
    }

    nlohmann::json root = nlohmann::json::parse(payload);
    nlohmann::json dataNode;
    if (cfg.dataField && root.contains(*cfg.dataField)) {
        dataNode = root.at(*cfg.dataField);
    } else {
        dataNode = root;
    }

    if (!dataNode.is_array()) {
        throw std::runtime_error("API response does not contain an array of candles");
    }

    candles.reserve(dataNode.size());
    for (const auto& entry : dataNode) {
        try {
            Candle candle;
            if (!parseTimestampField(entry, cfg.fields.timestamp, candle.timestamp)) {
                spdlog::warn("Skipping API candle with invalid timestamp");
                continue;
            }

            candle.open = getNumericField(entry, cfg.fields.open);
            candle.high = getNumericField(entry, cfg.fields.high);
            candle.low = getNumericField(entry, cfg.fields.low);
            candle.close = getNumericField(entry, cfg.fields.close);

            try {
                candle.volume = cfg.fields.volume.empty() ? 0.0 : getNumericField(entry, cfg.fields.volume);
            } catch (const std::exception&) {
                candle.volume = 0.0;
            }

            candle.symbol = getSymbolField(entry, cfg);

            candles.push_back(candle);
        } catch (const std::exception& ex) {
            spdlog::warn("Skipping API candle due to parse error: {}", ex.what());
        }
    }
    return candles;
}

} // namespace fastquant
