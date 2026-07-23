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

// One rule file after the discovery/parse pass, before validation. Parse
// failures are retained so validation can convert them into RejectedRule
// entries without re-reading the file.
class DiscoveredRule {
public:
    static DiscoveredRule FromParsed(std::filesystem::path source, Rule rule);
    static DiscoveredRule FromParseError(std::filesystem::path source, std::string error);

    const std::filesystem::path& Source() const;
    bool ParseSucceeded() const;
    const Rule& ParsedRule() const;
    const std::string& ParseError() const;

private:
    DiscoveredRule(std::filesystem::path source, Rule rule, bool parseSucceeded,
                   std::string parseError);

    std::filesystem::path source_;
    Rule rule_;
    bool parseSucceeded_;
    std::string parseError_;
};

// Loads every *.json rule file from a directory, parses it, and validates
// it. Discovery/parsing and validation are exposed as separate steps so the
// Application can time them independently via PerformanceProfiler.
// LoadDirectory() remains the single-call convenience that runs both.
class RuleLoader {
public:
    std::vector<DiscoveredRule> DiscoverAndParse(const std::filesystem::path& directory) const;
    RuleLoadResult ValidateRules(const std::vector<DiscoveredRule>& discovered) const;
    RuleLoadResult LoadDirectory(const std::filesystem::path& directory) const;

private:
    RuleParser ruleParser_;
    RuleValidator ruleValidator_;
};

}  // namespace sentinelforge
