#include "CSVDataLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace fastquant {

static bool parseISO8601ToTimePoint(const std::string& s, std::chrono::system_clock::time_point& out)
{
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) return false;

    // timegm is available on POSIX; on Windows _mkgmtime is used.
#if defined(_WIN32)
    time_t tt = _mkgmtime(&tm);
#else
    time_t tt = timegm(&tm);
#endif
    if (tt == -1) return false;
    out = std::chrono::system_clock::from_time_t(tt);
    return true;
}

std::vector<Candle> CSVDataLoader::load(const std::string& path, const Config& cfg) const
{
    std::vector<Candle> out;
    std::ifstream ifs(path);
    if (!ifs.is_open()) throw std::runtime_error("Failed to open CSV file: " + path);

    std::string line;
    size_t lineNo = 0;

    if (cfg.hasHeader) {
        if (!std::getline(ifs, line)) return out; // empty file
        ++lineNo;
    }

    while (std::getline(ifs, line)) {
        ++lineNo;
        if (line.empty()) continue;

        try {
            std::vector<std::string> fields;
            fields.reserve(8);
            std::string cur;
            std::istringstream ls(line);
            while (std::getline(ls, cur, cfg.delimiter)) fields.push_back(cur);

            // trim whitespace (including CR) from fields
            auto trim = [](std::string &s) {
                // remove leading
                size_t start = 0;
                while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) ++start;
                size_t end = s.size();
                while (end > start && isspace(static_cast<unsigned char>(s[end-1]))) --end;
                if (start == 0 && end == s.size()) return;
                s = s.substr(start, end - start);
            };
            for (auto &f : fields) trim(f);

            // Accept at least 6 fields: timestamp,open,high,low,close,volume
            if (fields.size() < 6) {
                if (cfg.strict) throw std::runtime_error("Not enough fields");
                std::cerr << "[warn] Skipping line " << lineNo << " (not enough fields)\n";
                continue;
            }

            Candle c;
            // timestamp
            if (!parseISO8601ToTimePoint(fields[0], c.timestamp)) {
                if (cfg.strict) throw std::runtime_error("Invalid timestamp format");
                std::cerr << "[warn] Skipping line " << lineNo << " (invalid timestamp): " << fields[0] << "\n";
                continue;
            }

            // numeric conversions
            auto to_double = [&](const std::string& s)->double {
                std::size_t idx = 0;
                double v = std::stod(s, &idx);
                if (idx != s.size()) throw std::invalid_argument("extra chars in number");
                return v;
            };

            c.open = to_double(fields[1]);
            c.high = to_double(fields[2]);
            c.low  = to_double(fields[3]);
            c.close= to_double(fields[4]);
            c.volume = to_double(fields[5]);

            if (fields.size() >= 7) c.symbol = fields[6];

            out.push_back(std::move(c));
        } catch (const std::exception& e) {
            if (cfg.strict) throw;
            std::cerr << "[warn] Skipping malformed row " << lineNo << ": " << e.what() << "\n";
            continue;
        }
    }

    return out;
}

} // namespace fastquant
