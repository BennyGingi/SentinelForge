#include "Scenario.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace sentinelforge {

Scenario::Scenario(std::string name, std::string description,
                   std::vector<std::string> mitreTechniques, std::vector<nlohmann::json> events,
                   std::vector<std::string> expectedDetections)
    : name_(std::move(name)),
      description_(std::move(description)),
      mitreTechniques_(std::move(mitreTechniques)),
      events_(std::move(events)),
      expectedDetections_(std::move(expectedDetections)) {}

const std::string& Scenario::Name() const { return name_; }
const std::string& Scenario::Description() const { return description_; }
const std::vector<std::string>& Scenario::MitreTechniques() const { return mitreTechniques_; }
const std::vector<nlohmann::json>& Scenario::Events() const { return events_; }
const std::vector<std::string>& Scenario::ExpectedDetections() const {
    return expectedDetections_;
}

namespace {

Scenario EmptyScenario() { return Scenario("", "", {}, {}, {}); }

std::string ScalarOrEmpty(const YAML::Node& node) {
    if (!node || !node.IsScalar()) {
        return "";
    }
    return node.as<std::string>();
}

std::vector<std::string> StringList(const YAML::Node& node) {
    std::vector<std::string> values;
    if (!node || !node.IsSequence()) {
        return values;
    }
    for (const auto& item : node) {
        if (item.IsScalar()) {
            values.push_back(item.as<std::string>());
        }
    }
    return values;
}

// One scenario event, in the snake_case shape EventNormalizer::NormalizeJson
// consumes. Unrecognized keys are carried through harmlessly; NormalizeJson
// ignores keys it doesn't know.
nlohmann::json EventToJson(const YAML::Node& eventNode) {
    nlohmann::json json = nlohmann::json::object();
    if (!eventNode || !eventNode.IsMap()) {
        return json;
    }
    for (const auto& entry : eventNode) {
        if (!entry.first.IsScalar() || !entry.second.IsScalar()) {
            continue;
        }
        json[entry.first.as<std::string>()] = entry.second.as<std::string>();
    }
    return json;
}

}  // namespace

ScenarioLoadResult ScenarioLoadResult::Success(Scenario scenario) {
    return ScenarioLoadResult(true, std::move(scenario), "", {});
}

ScenarioLoadResult ScenarioLoadResult::Failure(std::string identifier,
                                               std::vector<std::string> errors) {
    return ScenarioLoadResult(false, EmptyScenario(), std::move(identifier), std::move(errors));
}

ScenarioLoadResult::ScenarioLoadResult(bool valid, Scenario scenario, std::string identifier,
                                       std::vector<std::string> errors)
    : valid_(valid),
      scenario_(std::move(scenario)),
      identifier_(std::move(identifier)),
      errors_(std::move(errors)) {}

bool ScenarioLoadResult::IsValid() const { return valid_; }
const Scenario& ScenarioLoadResult::GetScenario() const { return scenario_; }
const std::string& ScenarioLoadResult::Identifier() const { return identifier_; }
const std::vector<std::string>& ScenarioLoadResult::Errors() const { return errors_; }

ScenarioLoadResult ScenarioLoader::LoadFile(const std::filesystem::path& path) const {
    const std::string displayName = path.filename().string();

    std::ifstream file(path);
    if (!file.is_open()) {
        return ScenarioLoadResult::Failure(displayName, {"could not open file"});
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    YAML::Node root;
    try {
        root = YAML::Load(buffer.str());
    } catch (const YAML::Exception& ex) {
        return ScenarioLoadResult::Failure(displayName, {std::string("YAML parse error: ") + ex.what()});
    }

    if (!root || !root.IsMap()) {
        return ScenarioLoadResult::Failure(displayName, {"root must be a mapping"});
    }

    std::vector<std::string> errors;

    const std::string name = ScalarOrEmpty(root["name"]);
    if (name.empty()) {
        errors.push_back("missing required field 'name'");
    }

    const std::string description = ScalarOrEmpty(root["description"]);
    const std::vector<std::string> mitreTechniques = StringList(root["mitre_techniques"]);

    std::vector<nlohmann::json> events;
    const YAML::Node eventsNode = root["events"];
    if (!eventsNode || !eventsNode.IsSequence() || eventsNode.size() == 0) {
        errors.push_back("missing or empty required field 'events'");
    } else {
        for (const auto& eventNode : eventsNode) {
            events.push_back(EventToJson(eventNode));
        }
    }

    // The key must be present as a sequence, but an empty sequence is a
    // legitimate, deliberate statement: "no rule/correlation covers this
    // technique yet" — a documented coverage gap, not a malformed file.
    const YAML::Node expectedNode = root["expected_detections"];
    if (!expectedNode || !expectedNode.IsSequence()) {
        errors.push_back(
            "missing required field 'expected_detections' (use [] to declare a documented "
            "coverage gap — no detection expected)");
    }
    const std::vector<std::string> expectedDetections = StringList(expectedNode);

    if (!errors.empty()) {
        return ScenarioLoadResult::Failure(name.empty() ? displayName : name, std::move(errors));
    }

    return ScenarioLoadResult::Success(Scenario(name, description, mitreTechniques,
                                                std::move(events), expectedDetections));
}

std::vector<std::filesystem::path> ScenarioLoader::DiscoverFiles(
    const std::filesystem::path& directory) const {
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        return files;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string ext = entry.path().extension().string();
        if (ext == ".yaml" || ext == ".yml") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace sentinelforge
