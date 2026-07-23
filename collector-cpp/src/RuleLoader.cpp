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

DiscoveredRule DiscoveredRule::FromParsed(std::filesystem::path source, Rule rule) {
    return DiscoveredRule(std::move(source), std::move(rule), true, "");
}

DiscoveredRule DiscoveredRule::FromParseError(std::filesystem::path source, std::string error) {
    return DiscoveredRule(std::move(source), Rule("", "", "", "", "", "", "", "", ""), false,
                          std::move(error));
}

DiscoveredRule::DiscoveredRule(std::filesystem::path source, Rule rule, bool parseSucceeded,
                               std::string parseError)
    : source_(std::move(source)),
      rule_(std::move(rule)),
      parseSucceeded_(parseSucceeded),
      parseError_(std::move(parseError)) {}

const std::filesystem::path& DiscoveredRule::Source() const { return source_; }
bool DiscoveredRule::ParseSucceeded() const { return parseSucceeded_; }
const Rule& DiscoveredRule::ParsedRule() const { return rule_; }
const std::string& DiscoveredRule::ParseError() const { return parseError_; }

std::vector<DiscoveredRule> RuleLoader::DiscoverAndParse(
    const std::filesystem::path& directory) const {
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

    std::vector<DiscoveredRule> discovered;
    discovered.reserve(ruleFiles.size());

    for (const auto& file : ruleFiles) {
        try {
            discovered.push_back(DiscoveredRule::FromParsed(file, ruleParser_.ParseFile(file)));
        } catch (const std::exception& e) {
            discovered.push_back(DiscoveredRule::FromParseError(file, e.what()));
        }
    }

    return discovered;
}

RuleLoadResult RuleLoader::ValidateRules(const std::vector<DiscoveredRule>& discovered) const {
    std::vector<Rule> accepted;
    std::vector<RejectedRule> rejected;
    std::unordered_set<std::string> acceptedNames;

    for (const auto& item : discovered) {
        if (!item.ParseSucceeded()) {
            rejected.emplace_back(item.Source().filename().string(),
                                  std::vector<std::string>{item.ParseError()});
            continue;
        }

        const Rule& rule = item.ParsedRule();
        const RuleValidationResult validation = ruleValidator_.Validate(rule, acceptedNames);

        if (validation.IsValid()) {
            acceptedNames.insert(rule.RuleName());
            accepted.push_back(rule);
        } else {
            rejected.emplace_back(DisplayName(rule, item.Source()), validation.Errors());
        }
    }

    return RuleLoadResult(std::move(accepted), std::move(rejected));
}

RuleLoadResult RuleLoader::LoadDirectory(const std::filesystem::path& directory) const {
    return ValidateRules(DiscoverAndParse(directory));
}

}  // namespace sentinelforge
