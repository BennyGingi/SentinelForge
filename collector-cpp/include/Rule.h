#pragma once

#include <string>

namespace sentinelforge {

// Immutable representation of one detection rule loaded from rules/*.json.
// Required fields (name, severity, mitre, and the process/command-line
// condition pair) are enforced by RuleValidator, not by this class — Rule
// itself just carries whatever was present in the source file, including
// empty strings for absent fields.
class Rule {
public:
    Rule(std::string ruleName,
         std::string processName,
         std::string commandLineContains,
         std::string severity,
         std::string mitre,
         std::string author,
         std::string version,
         std::string description,
         std::string createdDate);

    // Required
    const std::string& RuleName() const;
    const std::string& ProcessName() const;
    const std::string& CommandLineContains() const;
    const std::string& Severity() const;
    const std::string& Mitre() const;

    // Optional metadata
    const std::string& Author() const;
    const std::string& Version() const;
    const std::string& Description() const;
    const std::string& CreatedDate() const;

private:
    std::string ruleName_;
    std::string processName_;
    std::string commandLineContains_;
    std::string severity_;
    std::string mitre_;
    std::string author_;
    std::string version_;
    std::string description_;
    std::string createdDate_;
};

}  // namespace sentinelforge
