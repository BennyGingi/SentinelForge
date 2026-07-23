#include "SigmaTranslator.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace sentinelforge {

namespace {

std::string MapLevel(const std::string& level) {
    std::string lower = level;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "critical") return "Critical";
    if (lower == "high") return "High";
    if (lower == "medium") return "Medium";
    if (lower == "low" || lower == "informational") return "Low";
    // Pass through unchanged so RuleValidator can reject unknown values.
    if (level.empty()) {
        return "";
    }
    // Capitalize first letter as a best-effort mapping for uncommon levels.
    std::string mapped = lower;
    mapped[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(mapped[0])));
    return mapped;
}

// Prefer tags like attack.t1059.001 → T1059.001. Falls back to empty when no
// technique tag is present (RuleValidator will then reject the Rule).
std::string ExtractMitre(const std::vector<std::string>& tags) {
    for (const auto& tag : tags) {
        std::string lower = tag;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        static constexpr char kPrefix[] = "attack.t";
        if (lower.rfind(kPrefix, 0) != 0) {
            continue;
        }
        std::string technique = lower.substr(sizeof(kPrefix) - 1);  // after "attack.t"
        if (technique.empty()) {
            continue;
        }
        technique[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(technique[0])));
        return "T" + technique;
    }
    return "";
}

}  // namespace

Rule SigmaTranslator::Translate(const SigmaRule& sigmaRule) const {
    return Rule(sigmaRule.Title(), sigmaRule.ProcessName(), sigmaRule.CommandLineContains(),
                MapLevel(sigmaRule.Level()), ExtractMitre(sigmaRule.Tags()), "", "",
                sigmaRule.Description(), "");
}

}  // namespace sentinelforge
