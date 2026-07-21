#include "Rule.h"

#include <utility>

namespace sentinelforge {

Rule::Rule(std::string ruleName,
           std::string processName,
           std::string commandLineContains,
           std::string severity,
           std::string mitre,
           std::string author,
           std::string version,
           std::string description,
           std::string createdDate)
    : ruleName_(std::move(ruleName)),
      processName_(std::move(processName)),
      commandLineContains_(std::move(commandLineContains)),
      severity_(std::move(severity)),
      mitre_(std::move(mitre)),
      author_(std::move(author)),
      version_(std::move(version)),
      description_(std::move(description)),
      createdDate_(std::move(createdDate)) {}

const std::string& Rule::RuleName() const { return ruleName_; }
const std::string& Rule::ProcessName() const { return processName_; }
const std::string& Rule::CommandLineContains() const { return commandLineContains_; }
const std::string& Rule::Severity() const { return severity_; }
const std::string& Rule::Mitre() const { return mitre_; }
const std::string& Rule::Author() const { return author_; }
const std::string& Rule::Version() const { return version_; }
const std::string& Rule::Description() const { return description_; }
const std::string& Rule::CreatedDate() const { return createdDate_; }

}  // namespace sentinelforge
