#include "SigmaRule.h"

#include <utility>

namespace sentinelforge {

SigmaRule::SigmaRule(std::string title,
                     std::string id,
                     std::string status,
                     std::string description,
                     std::string logsourceCategory,
                     std::string processName,
                     std::string commandLineContains,
                     std::string condition,
                     std::string level,
                     std::vector<std::string> tags)
    : title_(std::move(title)),
      id_(std::move(id)),
      status_(std::move(status)),
      description_(std::move(description)),
      logsourceCategory_(std::move(logsourceCategory)),
      processName_(std::move(processName)),
      commandLineContains_(std::move(commandLineContains)),
      condition_(std::move(condition)),
      level_(std::move(level)),
      tags_(std::move(tags)) {}

const std::string& SigmaRule::Title() const { return title_; }
const std::string& SigmaRule::Id() const { return id_; }
const std::string& SigmaRule::Status() const { return status_; }
const std::string& SigmaRule::Description() const { return description_; }
const std::string& SigmaRule::LogsourceCategory() const { return logsourceCategory_; }
const std::string& SigmaRule::ProcessName() const { return processName_; }
const std::string& SigmaRule::CommandLineContains() const { return commandLineContains_; }
const std::string& SigmaRule::Condition() const { return condition_; }
const std::string& SigmaRule::Level() const { return level_; }
const std::vector<std::string>& SigmaRule::Tags() const { return tags_; }

}  // namespace sentinelforge
