#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "Rule.h"
#include "RuleParser.h"
#include "RuleValidator.h"

namespace sentinelforge {

// One rule file that did not make it into the accepted set: an identifying
// label (its rule name if known, otherwise its filename) and every reason
// it was rejected.
class RejectedRule {
public:
    RejectedRule(std::string identifier, std::vector<std::string> errors);

    const std::string& Identifier() const;
    const std::vector<std::string>& Errors() const;

private:
    std::string identifier_;
    std::vector<std::string> errors_;
};

// Outcome of loading a rules directory: every accepted Rule, plus details
// of every rejection. Discovered = Accepted + Rejected.
class RuleLoadResult {
public:
    RuleLoadResult(std::vector<Rule> accepted, std::vector<RejectedRule> rejected);

    const std::vector<Rule>& Accepted() const;
    const std::vector<RejectedRule>& Rejected() const;
    std::size_t Discovered() const;

private:
    std::vector<Rule> accepted_;
    std::vector<RejectedRule> rejected_;
};

// Loads every *.json rule file from a directory, parses it, and validates
// it. Only valid rules are returned in RuleLoadResult::Accepted(); invalid
// rules are never handed to the caller as usable Rule objects.
class RuleLoader {
public:
    RuleLoadResult LoadDirectory(const std::filesystem::path& directory) const;

private:
    RuleParser ruleParser_;
    RuleValidator ruleValidator_;
};

}  // namespace sentinelforge
