#pragma once

#include "Candle.h"
#include <string>
#include <vector>

namespace fastquant {

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
};

} // namespace fastquant
