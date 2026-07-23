#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

#include "Logger.h"
#include "RuleLoader.h"
#include "SigmaParser.h"
#include "SigmaTranslator.h"

namespace sentinelforge {

// Runtime settings for Sigma rule loading. Owned by Configuration.
struct SigmaSettings {
    bool enabled = true;
    std::filesystem::path rulesDirectory{"sigma-rules"};
};

// Discovers Sigma YAML rules, parses/validates them, translates each into an
// internal Rule, and runs RuleValidator so accepted Rules are indistinguishable
// from native JSON rules once they reach DetectionEngine.
class SigmaLoader {
public:
    RuleLoadResult LoadDirectory(const std::filesystem::path& directory,
                                 const std::unordered_set<std::string>& existingRuleNames,
                                 const Logger& logger) const;

private:
    SigmaParser parser_;
    SigmaTranslator translator_;
    RuleValidator ruleValidator_;
};

}  // namespace sentinelforge
