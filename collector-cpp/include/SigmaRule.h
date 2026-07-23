#pragma once

#include <string>
#include <vector>

namespace sentinelforge {

// Intermediate representation of a Sigma rule after YAML parsing and before
// translation into the internal Rule model. Holds only the Phase-1 subset;
// unsupported Sigma fields are ignored by the parser and never appear here.
class SigmaRule {
public:
    SigmaRule(std::string title,
              std::string id,
              std::string status,
              std::string description,
              std::string logsourceCategory,
              std::string processName,
              std::string commandLineContains,
              std::string condition,
              std::string level,
              std::vector<std::string> tags);

    const std::string& Title() const;
    const std::string& Id() const;
    const std::string& Status() const;
    const std::string& Description() const;
    const std::string& LogsourceCategory() const;
    const std::string& ProcessName() const;
    const std::string& CommandLineContains() const;
    const std::string& Condition() const;
    const std::string& Level() const;
    const std::vector<std::string>& Tags() const;

private:
    std::string title_;
    std::string id_;
    std::string status_;
    std::string description_;
    std::string logsourceCategory_;
    std::string processName_;
    std::string commandLineContains_;
    std::string condition_;
    std::string level_;
    std::vector<std::string> tags_;
};

}  // namespace sentinelforge
