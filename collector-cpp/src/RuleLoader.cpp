#include "RuleLoader.h"

#include <algorithm>
#include <stdexcept>

namespace sentinelforge {

std::vector<Rule> RuleLoader::LoadDirectory(const std::filesystem::path& directory) const {
    if (!std::filesystem::is_directory(directory)) {
        throw std::runtime_error("Rule directory not found: " + directory.string());
    }

    std::vector<std::filesystem::path> ruleFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            ruleFiles.push_back(entry.path());
        }
    }

    // Directory iteration order is unspecified; sort for deterministic output.
    std::sort(ruleFiles.begin(), ruleFiles.end());

    std::vector<Rule> rules;
    rules.reserve(ruleFiles.size());
    for (const auto& file : ruleFiles) {
        rules.push_back(ruleParser_.ParseFile(file));
    }
    return rules;
}

}  // namespace sentinelforge
