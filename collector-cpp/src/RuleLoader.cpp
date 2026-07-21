#include "RuleLoader.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace sentinelforge {

namespace {

std::string DisplayName(const Rule& rule, const std::filesystem::path& file) {
    return rule.RuleName().empty() ? file.filename().string() : rule.RuleName();
}

}  // namespace

RejectedRule::RejectedRule(std::string identifier, std::vector<std::string> errors)
    : identifier_(std::move(identifier)), errors_(std::move(errors)) {}

const std::string& RejectedRule::Identifier() const { return identifier_; }
const std::vector<std::string>& RejectedRule::Errors() const { return errors_; }

RuleLoadResult::RuleLoadResult(std::vector<Rule> accepted, std::vector<RejectedRule> rejected)
    : accepted_(std::move(accepted)), rejected_(std::move(rejected)) {}

const std::vector<Rule>& RuleLoadResult::Accepted() const { return accepted_; }
const std::vector<RejectedRule>& RuleLoadResult::Rejected() const { return rejected_; }
std::size_t RuleLoadResult::Discovered() const { return accepted_.size() + rejected_.size(); }

RuleLoadResult RuleLoader::LoadDirectory(const std::filesystem::path& directory) const {
    if (!std::filesystem::is_directory(directory)) {
        throw std::runtime_error("Rule directory not found: " + directory.string());
    }

    std::vector<std::filesystem::path> ruleFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            ruleFiles.push_back(entry.path());
        }
    }

    // Directory iteration order is unspecified; sort for deterministic output.
    std::sort(ruleFiles.begin(), ruleFiles.end());

    std::vector<Rule> accepted;
    std::vector<RejectedRule> rejected;
    std::unordered_set<std::string> acceptedNames;

    for (const auto& file : ruleFiles) {
        try {
            const Rule rule = ruleParser_.ParseFile(file);
            const RuleValidationResult validation = ruleValidator_.Validate(rule, acceptedNames);

            if (validation.IsValid()) {
                acceptedNames.insert(rule.RuleName());
                accepted.push_back(rule);
            } else {
                rejected.emplace_back(DisplayName(rule, file), validation.Errors());
            }
        } catch (const std::exception& e) {
            rejected.emplace_back(file.filename().string(), std::vector<std::string>{e.what()});
        }
    }

    return RuleLoadResult(std::move(accepted), std::move(rejected));
}

}  // namespace sentinelforge
