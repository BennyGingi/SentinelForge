#pragma once

#include <string>

namespace sentinelforge {

// Immutable representation of one detection rule loaded from rules/*.json.
class Rule {
public:
    Rule(std::string ruleName,
         std::string processName,
         std::string commandLineContains,
         std::string severity,
         std::string mitre);

    const std::string& RuleName() const;
    const std::string& ProcessName() const;
    const std::string& CommandLineContains() const;
    const std::string& Severity() const;
    const std::string& Mitre() const;

private:
    std::string ruleName_;
    std::string processName_;
    std::string commandLineContains_;
    std::string severity_;
    std::string mitre_;
};

}  // namespace sentinelforge
