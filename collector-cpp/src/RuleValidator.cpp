#include "RuleValidator.h"

#include <algorithm>
#include <array>
#include <utility>

namespace sentinelforge {

namespace {

constexpr std::array<const char*, 4> kAllowedSeverities = {"Low", "Medium", "High", "Critical"};

bool IsAllowedSeverity(const std::string& severity) {
    return std::any_of(kAllowedSeverities.begin(), kAllowedSeverities.end(),
                        [&severity](const char* allowed) { return severity == allowed; });
}

std::string AllowedSeveritiesList() {
    return "Low, Medium, High, Critical";
}

}  // namespace

RuleValidationResult::RuleValidationResult(bool valid, std::vector<std::string> errors)
    : valid_(valid), errors_(std::move(errors)) {}

bool RuleValidationResult::IsValid() const { return valid_; }
const std::vector<std::string>& RuleValidationResult::Errors() const { return errors_; }

RuleValidationResult RuleValidator::Validate(
    const Rule& rule, const std::unordered_set<std::string>& existingRuleNames) const {
    std::vector<std::string> errors;

    if (rule.RuleName().empty()) {
        errors.push_back("Missing required field: name");
    } else if (existingRuleNames.count(rule.RuleName()) > 0) {
        errors.push_back("Duplicate rule name: " + rule.RuleName());
    }

    if (rule.Severity().empty()) {
        errors.push_back("Missing required field: severity");
    } else if (!IsAllowedSeverity(rule.Severity())) {
        errors.push_back("Invalid severity: " + rule.Severity() + " (allowed: " +
                          AllowedSeveritiesList() + ")");
    }

    if (rule.Mitre().empty()) {
        errors.push_back("Missing required field: mitre");
    }

    const bool hasProcessName = !rule.ProcessName().empty();
    const bool hasCommandLine = !rule.CommandLineContains().empty();
    if (!hasProcessName && !hasCommandLine) {
        errors.push_back(
            "Missing condition block: rule must define process_name and command_line_contains");
    } else if (!hasProcessName) {
        errors.push_back("Missing required field: process_name");
    } else if (!hasCommandLine) {
        errors.push_back("Missing required field: command_line_contains");
    }

    const bool valid = errors.empty();
    return RuleValidationResult(valid, std::move(errors));
}

}  // namespace sentinelforge
