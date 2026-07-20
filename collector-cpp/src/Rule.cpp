#include "Rule.h"

#include <utility>

namespace sentinelforge {

Rule::Rule(std::string ruleName,
           std::string processName,
           std::string commandLineContains,
           std::string severity,
           std::string mitre)
    : ruleName_(std::move(ruleName)),
      processName_(std::move(processName)),
      commandLineContains_(std::move(commandLineContains)),
      severity_(std::move(severity)),
      mitre_(std::move(mitre)) {}

const std::string& Rule::RuleName() const { return ruleName_; }
const std::string& Rule::ProcessName() const { return processName_; }
const std::string& Rule::CommandLineContains() const { return commandLineContains_; }
const std::string& Rule::Severity() const { return severity_; }
const std::string& Rule::Mitre() const { return mitre_; }

}  // namespace sentinelforge
