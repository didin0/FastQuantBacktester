#include "EnvLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace fastquant {
namespace app {
namespace {

std::string trim(const std::string& input) {
    auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

#ifdef _WIN32
bool setEnv(const std::string& key, const std::string& value) {
    return _putenv_s(key.c_str(), value.c_str()) == 0;
}
#else
bool setEnv(const std::string& key, const std::string& value) {
    return setenv(key.c_str(), value.c_str(), 1) == 0;
}
#endif

std::string stripQuotes(const std::string& value) {
    if (value.size() >= 2) {
        char first = value.front();
        char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::string lookupEnv(const std::string& key) {
    if (key.empty()) {
        return {};
    }
#if defined(_WIN32)
    char* buffer = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buffer, &len, key.c_str()) != 0 || buffer == nullptr) {
        return {};
    }
    std::string result(buffer);
    free(buffer);
    return result;
#else
    if (const char* value = std::getenv(key.c_str())) {
        return value;
    }
    return {};
#endif
}

} // namespace

bool loadEnvFile(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return false;
    }
    std::ifstream ifs(path);
    if (!ifs.good()) {
        return false;
    }
    bool loadedAny = false;
    std::string line;
    while (std::getline(ifs, line)) {
        auto trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        auto pos = trimmed.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        auto key = trim(trimmed.substr(0, pos));
        auto value = trim(trimmed.substr(pos + 1));
        if (key.empty()) {
            continue;
        }
        value = stripQuotes(value);
        if (setEnv(key, value)) {
            loadedAny = true;
        }
    }
    return loadedAny;
}

std::string expandEnvVars(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    size_t pos = 0;
    while (pos < input.size()) {
        auto start = input.find("${", pos);
        if (start == std::string::npos) {
            output.append(input.substr(pos));
            break;
        }
        output.append(input.substr(pos, start - pos));
        auto end = input.find('}', start + 2);
        if (end == std::string::npos) {
            output.append(input.substr(start));
            break;
        }
        auto varName = input.substr(start + 2, end - (start + 2));
        output.append(lookupEnv(trim(varName)));
        pos = end + 1;
    }
    return output;
}

} // namespace app
} // namespace fastquant
