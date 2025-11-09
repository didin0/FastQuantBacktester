#pragma once

#include "Candle.h"
#include <string>
#include <vector>
#include <functional>
#include <type_traits>
#include "csv.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

namespace fastquant {

// Parse ISO8601 timestamp (YYYY-MM-DDTHH:MM:SSZ) to time_point.
inline bool parseISO8601ToTimePoint(const std::string& s, std::chrono::system_clock::time_point& out)
{
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) return false;

#if defined(_WIN32)
    time_t tt = _mkgmtime(&tm);
#else
    time_t tt = timegm(&tm);
#endif
    if (tt == -1) return false;
    out = std::chrono::system_clock::from_time_t(tt);
    return true;
}


class CSVDataLoader {
public:
    struct Config {
        char delimiter = ',';
        bool hasHeader = true;
        bool strict = false; // strict parsing: throw on malformed rows
        std::string timestampFormat = "%Y-%m-%dT%H:%M:%SZ"; // ISO8601 by default
    };

    CSVDataLoader() = default;

    // Load entire CSV into memory (std::vector<Candle>)
    std::vector<Candle> load(const std::string& path, const Config& cfg) const;

    // Convenience overload that uses default Config
    std::vector<Candle> load(const std::string& path) const { return load(path, Config()); }

    // Stream rows: invoke callback for each parsed Candle. Callback returns bool: true=continue, false=stop.
    // Non-template overload (std::function) - forwards to the templated implementation.
    void stream(const std::string& path, const std::function<bool(const Candle&)>& cb, const Config& cfg) const;

    // Convenience overload
    void stream(const std::string& path, const std::function<bool(const Candle&)>& cb) const { stream(path, cb, Config()); }

    // Template overload: accepts any callable F(Candle) -> bool (or convertible to bool) with no std::function allocation.
    template<typename F>
    auto stream(const std::string& path, F&& cb, const Config& cfg) const
        -> std::enable_if_t<std::is_invocable_r_v<bool, F, const Candle&>, void>
    {
        // Try 7-column reader
        try {
            io::CSVReader<7> in(path);
            if (cfg.hasHeader) in.read_header(io::ignore_extra_column, "timestamp", "open", "high", "low", "close", "volume", "symbol");

            std::string ts;
            std::string open_s, high_s, low_s, close_s, volume_s, symbol;

            while (in.read_row(ts, open_s, high_s, low_s, close_s, volume_s, symbol)) {
                Candle c;
                if (!fastquant::parseISO8601ToTimePoint(ts, c.timestamp)) {
                    if (cfg.strict) throw std::runtime_error("Invalid timestamp format: " + ts);
                    spdlog::warn("Skipping row (invalid timestamp): {}", ts);
                    continue;
                }
                try {
                    c.open = std::stod(open_s);
                    c.high = std::stod(high_s);
                    c.low = std::stod(low_s);
                    c.close = std::stod(close_s);
                    c.volume = std::stod(volume_s);
                    c.symbol = symbol;
                } catch (const std::exception& ex) {
                    if (cfg.strict) throw;
                    spdlog::warn("Skipping malformed numeric row: {}", ex.what());
                    continue;
                }

                if (!cb(c)) return; // early stop
            }

            return;
        } catch (...) {
            // fallback to 6-column reader
        }

        io::CSVReader<6> in2(path);
        if (cfg.hasHeader) in2.read_header(io::ignore_extra_column, "timestamp", "open", "high", "low", "close", "volume");

        std::string ts;
        std::string open_s, high_s, low_s, close_s, volume_s;

        while (in2.read_row(ts, open_s, high_s, low_s, close_s, volume_s)) {
            Candle c;
            if (!fastquant::parseISO8601ToTimePoint(ts, c.timestamp)) {
                if (cfg.strict) throw std::runtime_error("Invalid timestamp format: " + ts);
                spdlog::warn("Skipping row (invalid timestamp): {}", ts);
                continue;
            }
            try {
                c.open = std::stod(open_s);
                c.high = std::stod(high_s);
                c.low = std::stod(low_s);
                c.close = std::stod(close_s);
                c.volume = std::stod(volume_s);
            } catch (const std::exception& ex) {
                if (cfg.strict) throw;
                spdlog::warn("Skipping malformed numeric row: {}", ex.what());
                continue;
            }

            if (!cb(c)) return;
        }
    }
};

} // namespace fastquant
