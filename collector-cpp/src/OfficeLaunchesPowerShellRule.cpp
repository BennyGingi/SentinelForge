#include "OfficeLaunchesPowerShellRule.h"

#include <algorithm>
#include <cctype>
#include <chrono>

namespace sentinelforge {

namespace {

constexpr std::chrono::seconds kChainWindow{60};

std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool EqualsIgnoreCase(const std::string& lhs, const std::string& rhs) {
    return ToLower(lhs) == ToLower(rhs);
}

bool IsPowerShell(const std::string& processName) {
    const std::string lower = ToLower(processName);
    return lower == "powershell.exe" || lower == "pwsh.exe";
}

bool IsOfficeProcess(const std::string& processName) {
    const std::string lower = ToLower(processName);
    return lower == "winword.exe" || lower == "excel.exe" || lower == "powerpnt.exe" ||
           lower == "outlook.exe" || lower == "onenote.exe" || lower == "msaccess.exe";
}

}  // namespace

std::string OfficeLaunchesPowerShellRule::Id() const {
    return "office_launches_powershell";
}

std::optional<CorrelationAlert> OfficeLaunchesPowerShellRule::Evaluate(
    const NormalizedEvent& current,
    const std::deque<CorrelationHistoryEntry>& history,
    const std::vector<DetectionResult>& /*detectionResults*/) const {
    if (!IsPowerShell(current.ProcessName())) {
        return std::nullopt;
    }

    const auto now = std::chrono::system_clock::now();
    const CorrelationHistoryEntry* officeEntry = nullptr;

    // Prefer an Office parent on the current event (direct spawn chain).
    if (IsOfficeProcess(current.ParentProcess())) {
        for (auto it = history.rbegin(); it != history.rend(); ++it) {
            if (!EqualsIgnoreCase(it->event.ProcessName(), current.ParentProcess())) {
                continue;
            }
            if (now - it->observedAt <= kChainWindow) {
                officeEntry = &(*it);
                break;
            }
        }
        // Parent matched even if the parent event is not in history yet.
        if (officeEntry == nullptr) {
            std::vector<NormalizedEvent> contributors;
            contributors.push_back(NormalizedEvent(
                current.Timestamp(), current.Hostname(), current.Username(), current.EventType(),
                current.ParentProcess(), "", "", "", "", "", "", "", "", current.Provider()));
            contributors.push_back(current);
            const std::size_t matchedCount = contributors.size();

            return CorrelationAlert(
                "Office application launches PowerShell",
                "PowerShell was launched with parent process '" + current.ParentProcess() +
                    "' within the Office→PowerShell correlation window.",
                "High", 85, current.Timestamp().empty() ? "unknown" : current.Timestamp(),
                matchedCount, {"T1059.001", "T1204.002"}, std::move(contributors));
        }
    }

    // Otherwise look for a recent Office process in history (same host when known).
    if (officeEntry == nullptr) {
        for (auto it = history.rbegin(); it != history.rend(); ++it) {
            if (!IsOfficeProcess(it->event.ProcessName())) {
                continue;
            }
            if (now - it->observedAt > kChainWindow) {
                continue;
            }
            if (!current.Hostname().empty() && !it->event.Hostname().empty() &&
                !EqualsIgnoreCase(current.Hostname(), it->event.Hostname())) {
                continue;
            }
            officeEntry = &(*it);
            break;
        }
    }

    if (officeEntry == nullptr) {
        return std::nullopt;
    }

    std::vector<NormalizedEvent> contributors;
    contributors.push_back(officeEntry->event);
    contributors.push_back(current);
    const std::size_t matchedCount = contributors.size();

    return CorrelationAlert(
        "Office application launches PowerShell",
        "Office process '" + officeEntry->event.ProcessName() +
            "' was followed by PowerShell within 60 seconds.",
        "High", 90, current.Timestamp().empty() ? "unknown" : current.Timestamp(), matchedCount,
        {"T1059.001", "T1204.002"}, std::move(contributors));
}

}  // namespace sentinelforge
