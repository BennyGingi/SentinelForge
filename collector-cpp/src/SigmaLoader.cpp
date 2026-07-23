#include "SigmaLoader.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sentinelforge {

RuleLoadResult SigmaLoader::LoadDirectory(
    const std::filesystem::path& directory,
    const std::unordered_set<std::string>& existingRuleNames,
    const Logger& logger) const {
    if (!std::filesystem::is_directory(directory)) {
        throw std::runtime_error("Sigma rule directory not found: " + directory.string());
    }

    std::vector<std::filesystem::path> ruleFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto ext = entry.path().extension().string();
        if (ext == ".yml" || ext == ".yaml") {
            ruleFiles.push_back(entry.path());
        }
    }

    std::sort(ruleFiles.begin(), ruleFiles.end());

    std::vector<Rule> accepted;
    std::vector<RejectedRule> rejected;
    std::unordered_set<std::string> acceptedNames = existingRuleNames;

    for (const auto& file : ruleFiles) {
        logger.Info("SigmaLoader", "Sigma rule discovered: " + file.filename().string());

        const SigmaParseResult parsed = parser_.ParseFile(file);
        if (!parsed.IsValid()) {
            logger.Error("SigmaLoader", "Sigma rule rejected: " + parsed.Identifier());
            for (const auto& message : parsed.Errors()) {
                logger.Error("SigmaLoader", message);
            }
            rejected.emplace_back(parsed.Identifier(), parsed.Errors());
            continue;
        }

        const Rule rule = translator_.Translate(parsed.Rule());
        logger.Info("SigmaTranslator",
                    "Translation completed: " + parsed.Rule().Title() + " -> Rule");

        const RuleValidationResult validation = ruleValidator_.Validate(rule, acceptedNames);
        if (!validation.IsValid()) {
            logger.Error("SigmaLoader", "Sigma rule rejected: " + parsed.Identifier());
            for (const auto& message : validation.Errors()) {
                logger.Error("SigmaLoader", message);
            }
            rejected.emplace_back(parsed.Identifier(), validation.Errors());
            continue;
        }

        acceptedNames.insert(rule.RuleName());
        accepted.push_back(rule);
        logger.Info("SigmaLoader", "Sigma rule accepted: " + rule.RuleName());
    }

    return RuleLoadResult(std::move(accepted), std::move(rejected));
}

}  // namespace sentinelforge
