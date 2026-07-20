#pragma once

#include <filesystem>
#include <vector>

#include "Rule.h"
#include "RuleParser.h"

namespace sentinelforge {

// Loads every *.json rule file from a directory into Rule objects.
// Delegates parsing of each individual file to RuleParser; has no
// knowledge of Event or matching logic.
class RuleLoader {
public:
    std::vector<Rule> LoadDirectory(const std::filesystem::path& directory) const;

private:
    RuleParser ruleParser_;
};

}  // namespace sentinelforge
