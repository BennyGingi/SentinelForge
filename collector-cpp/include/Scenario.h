#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace sentinelforge {

// One declarative regression scenario loaded from sample-attacks/*.yaml.
// events are already in the snake_case shape EventNormalizer::NormalizeJson
// consumes (timestamp, hostname, process_name, command_line, ...).
//
// expected_detections entries are matched against either a native Rule's
// RuleName() (e.g. "Suspicious PowerShell") or a CorrelationAlert's Title()
// (e.g. "Office application launches PowerShell") — whichever fired.
// Rule/DetectionResult have no separate "id" field in the collector today;
// RuleName is the closest stable identifier, so that is what scenarios name.
class Scenario {
public:
    Scenario(std::string name, std::string description,
             std::vector<std::string> mitreTechniques, std::vector<nlohmann::json> events,
             std::vector<std::string> expectedDetections, bool knownGap, std::string gapReason);

    const std::string& Name() const;
    const std::string& Description() const;
    const std::vector<std::string>& MitreTechniques() const;
    const std::vector<nlohmann::json>& Events() const;
    const std::vector<std::string>& ExpectedDetections() const;

    // known_gap: true declares that no rule/correlation is expected to fire
    // for this scenario yet — a documented, intentional coverage gap rather
    // than a malformed or forgotten expectation. gap_reason is required
    // whenever known_gap is true; it's the whole point of marking it.
    bool KnownGap() const;
    const std::string& GapReason() const;

private:
    std::string name_;
    std::string description_;
    std::vector<std::string> mitreTechniques_;
    std::vector<nlohmann::json> events_;
    std::vector<std::string> expectedDetections_;
    bool knownGap_;
    std::string gapReason_;
};

// Outcome of loading and schema-checking one scenario file.
class ScenarioLoadResult {
public:
    static ScenarioLoadResult Success(Scenario scenario);
    static ScenarioLoadResult Failure(std::string identifier, std::vector<std::string> errors);

    bool IsValid() const;
    const Scenario& GetScenario() const;
    const std::string& Identifier() const;
    const std::vector<std::string>& Errors() const;

private:
    ScenarioLoadResult(bool valid, Scenario scenario, std::string identifier,
                       std::vector<std::string> errors);

    bool valid_;
    Scenario scenario_;
    std::string identifier_;
    std::vector<std::string> errors_;
};

// Parses one scenario YAML file. Required top-level keys: name, events
// (non-empty sequence of event maps), expected_detections (a sequence —
// empty is allowed, see Scenario::KnownGap). description and
// mitre_techniques are optional. known_gap and gap_reason are optional but
// gap_reason is required whenever known_gap: true is present.
class ScenarioLoader {
public:
    ScenarioLoadResult LoadFile(const std::filesystem::path& path) const;

    // Discovers every *.yaml / *.yml file directly under directory (no
    // recursion), sorted for deterministic run order.
    std::vector<std::filesystem::path> DiscoverFiles(const std::filesystem::path& directory) const;
};

}  // namespace sentinelforge
