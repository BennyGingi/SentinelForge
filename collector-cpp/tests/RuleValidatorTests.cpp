#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "Rule.h"
#include "RuleValidator.h"

namespace sentinelforge {
namespace {

// Builds a rule from just the fields RuleValidator inspects, filling the
// optional metadata with fixed placeholder values. Keeps each test free of
// the nine-argument Rule constructor and of irrelevant setup.
Rule MakeRule(const std::string& name,
              const std::string& processName,
              const std::string& commandLineContains,
              const std::string& severity,
              const std::string& mitre) {
    return Rule(name, processName, commandLineContains, severity, mitre,
                "SentinelForge", "1.0", "test rule", "2024-01-01");
}

// A rule that passes every validation check; individual tests invalidate one
// field at a time so failures are unambiguous.
Rule ValidRule() {
    return MakeRule("Suspicious PowerShell", "powershell.exe", "-enc", "High", "T1059.001");
}

bool ContainsError(const std::vector<std::string>& errors, const std::string& substring) {
    return std::any_of(errors.begin(), errors.end(), [&substring](const std::string& error) {
        return error.find(substring) != std::string::npos;
    });
}

class RuleValidatorTest : public ::testing::Test {
protected:
    RuleValidator validator_;
    std::unordered_set<std::string> existingNames_;
};

TEST_F(RuleValidatorTest, ValidRuleAccepted) {
    const RuleValidationResult result = validator_.Validate(ValidRule(), existingNames_);

    EXPECT_TRUE(result.IsValid()) << "A fully-specified rule should be accepted";
    EXPECT_TRUE(result.Errors().empty()) << "A valid rule should carry no errors";
}

TEST_F(RuleValidatorTest, MissingNameRejected) {
    const Rule rule = MakeRule("", "powershell.exe", "-enc", "High", "T1059.001");

    const RuleValidationResult result = validator_.Validate(rule, existingNames_);

    EXPECT_FALSE(result.IsValid()) << "A rule without a name must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "name"))
        << "Rejection should mention the missing name field";
}

TEST_F(RuleValidatorTest, MissingSeverityRejected) {
    const Rule rule = MakeRule("Suspicious PowerShell", "powershell.exe", "-enc", "", "T1059.001");

    const RuleValidationResult result = validator_.Validate(rule, existingNames_);

    EXPECT_FALSE(result.IsValid()) << "A rule without a severity must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "severity"))
        << "Rejection should mention the missing severity field";
}

TEST_F(RuleValidatorTest, InvalidSeverityRejected) {
    const Rule rule =
        MakeRule("Suspicious PowerShell", "powershell.exe", "-enc", "Extreme", "T1059.001");

    const RuleValidationResult result = validator_.Validate(rule, existingNames_);

    EXPECT_FALSE(result.IsValid()) << "An out-of-vocabulary severity must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "Invalid severity"))
        << "Rejection should identify the severity as invalid";
}

TEST_F(RuleValidatorTest, MissingMitreRejected) {
    const Rule rule = MakeRule("Suspicious PowerShell", "powershell.exe", "-enc", "High", "");

    const RuleValidationResult result = validator_.Validate(rule, existingNames_);

    EXPECT_FALSE(result.IsValid()) << "A rule without a MITRE mapping must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "mitre"))
        << "Rejection should mention the missing mitre field";
}

TEST_F(RuleValidatorTest, DuplicateRuleRejected) {
    existingNames_.insert("Suspicious PowerShell");

    const RuleValidationResult result = validator_.Validate(ValidRule(), existingNames_);

    EXPECT_FALSE(result.IsValid()) << "A rule whose name is already accepted must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "Duplicate"))
        << "Rejection should identify the name as a duplicate";
}

TEST_F(RuleValidatorTest, MissingProcessConditionRejected) {
    const Rule rule = MakeRule("Suspicious PowerShell", "", "-enc", "High", "T1059.001");

    const RuleValidationResult result = validator_.Validate(rule, existingNames_);

    EXPECT_FALSE(result.IsValid()) << "A rule without a process condition must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "process_name"))
        << "Rejection should mention the missing process_name condition";
}

TEST_F(RuleValidatorTest, MissingCommandLineConditionRejected) {
    const Rule rule = MakeRule("Suspicious PowerShell", "powershell.exe", "", "High", "T1059.001");

    const RuleValidationResult result = validator_.Validate(rule, existingNames_);

    EXPECT_FALSE(result.IsValid()) << "A rule without a command-line condition must be rejected";
    EXPECT_TRUE(ContainsError(result.Errors(), "command_line_contains"))
        << "Rejection should mention the missing command_line_contains condition";
}

}  // namespace
}  // namespace sentinelforge
