#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace fastquant {

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

inline bool parseTimestamp(const std::string& s, std::chrono::system_clock::time_point& out)
{
    if (parseISO8601ToTimePoint(s, out)) return true;

    try {
        size_t i = 0;
        while (i < s.size() && isspace(static_cast<unsigned char>(s[i]))) ++i;
        size_t j = s.size();
        while (j > i && isspace(static_cast<unsigned char>(s[j - 1]))) --j;
        if (i >= j) return false;
        std::string num = s.substr(i, j - i);

        long long v = std::stoll(num);

        if (std::llabs(v) > 100000000000LL) {
            out = std::chrono::system_clock::time_point(std::chrono::milliseconds(v));
        } else {
            out = std::chrono::system_clock::from_time_t(static_cast<time_t>(v));
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace fastquant
