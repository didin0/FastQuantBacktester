#pragma once

#include <filesystem>
#include <string>

namespace fastquant {
namespace app {

// Loads KEY=value pairs from the given file into the process environment.
// Lines starting with '#' are treated as comments. Returns true if at least
// one variable was set, false if the file was missing or empty.
bool loadEnvFile(const std::filesystem::path& path);

// Expands ${VAR_NAME} placeholders inside the provided string using the
// current environment, returning the expanded result. Unknown variables are
// replaced with an empty string.
std::string expandEnvVars(const std::string& input);

} // namespace app
} // namespace fastquant
