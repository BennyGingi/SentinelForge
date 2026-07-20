#include "DetectionEngine.h"

#include <algorithm>
#include <cctype>

namespace sentinelforge {

namespace {

std::string ToLower(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
}

bool EqualsCaseInsensitive(const std::string& lhs, const std::string& rhs) {
    return ToLower(lhs) == ToLower(rhs);
}

}  // namespace

DetectionResult DetectionEngine::Evaluate(const Event& event, const Rule& rule) const {
    const bool processMatches = EqualsCaseInsensitive(event.ProcessName(), rule.ProcessName());
    const bool commandLineMatches =
        ContainsCaseInsensitive(event.CommandLine(), rule.CommandLineContains());
    const bool matched = processMatches && commandLineMatches;

    std::string reason;
    if (matched) {
        reason = "process_name matched '" + rule.ProcessName() + "' and command_line contains '" +
                  rule.CommandLineContains() + "'";
    } else if (!processMatches) {
        reason = "process_name '" + event.ProcessName() + "' does not match '" + rule.ProcessName() + "'";
    } else {
        reason = "command_line does not contain '" + rule.CommandLineContains() + "'";
    }

    return DetectionResult(matched, rule.RuleName(), rule.Severity(), rule.Mitre(), reason);
}

}  // namespace sentinelforge
