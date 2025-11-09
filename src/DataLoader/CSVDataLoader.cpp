#include "CSVDataLoader.h"
#include <stdexcept>
#include <iomanip>

// fast-cpp-csv-parser header
#include "csv.h"
#include <iostream>
#include <spdlog/spdlog.h>

namespace fastquant {

std::vector<Candle> CSVDataLoader::load(const std::string& path, const Config& cfg) const
{
    std::vector<Candle> out;

    // Try reader with 7 columns (optional symbol)
    try {
    io::CSVReader<7> in(path);
    if (cfg.hasHeader) in.read_header(io::ignore_extra_column, "timestamp", "open", "high", "low", "close", "volume", "symbol");

        std::string ts;
        std::string open_s, high_s, low_s, close_s, volume_s, symbol;

        while (in.read_row(ts, open_s, high_s, low_s, close_s, volume_s, symbol)) {
            Candle c;
            if (!parseISO8601ToTimePoint(ts, c.timestamp)) {
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

            out.push_back(std::move(c));
        }

        return out;
    } catch (const std::exception& e) {
        // Fallback: try reading 6 columns (no symbol)
        try {
            io::CSVReader<6> in2(path);
            if (cfg.hasHeader) in2.read_header(io::ignore_extra_column, "timestamp", "open", "high", "low", "close", "volume");

            std::string ts;
            std::string open_s, high_s, low_s, close_s, volume_s;

            while (in2.read_row(ts, open_s, high_s, low_s, close_s, volume_s)) {
                Candle c;
                if (!parseISO8601ToTimePoint(ts, c.timestamp)) {
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
                out.push_back(std::move(c));
            }

            return out;
        } catch (const std::exception& e2) {
            throw std::runtime_error(std::string("Failed to parse CSV using fast-cpp-csv-parser: ") + e2.what());
        }
    }
}

// Note: stream template implementation is in the header to allow zero-overhead callables.
// Provide a std::function<bool(const Candle&)> overload that forwards to the header template.
void CSVDataLoader::stream(const std::string& path, const std::function<bool(const Candle&)>& cb, const Config& cfg) const
{
    // Forward to templated implementation in header by wrapping the std::function with a lambda
    // that satisfies the template invocation (this may allocate for std::function but keeps the header template zero-overhead usage for other callables).
    std::function<bool(const Candle&)> wrapper = cb;
    this->stream<std::function<bool(const Candle&)>>(path, std::move(wrapper), cfg);
}

} // namespace fastquant
