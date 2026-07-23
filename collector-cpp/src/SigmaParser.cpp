#include "SigmaParser.h"

#include <fstream>
#include <sstream>
#include <utility>

#include <yaml-cpp/yaml.h>

namespace sentinelforge {

namespace {

std::string ScalarOrEmpty(const YAML::Node& node) {
    if (!node || !node.IsScalar()) {
        return "";
    }
    return node.as<std::string>();
}

std::string OptionalMapString(const YAML::Node& map, const char* key) {
    if (!map || !map.IsMap() || !map[key]) {
        return "";
    }
    return ScalarOrEmpty(map[key]);
}

std::vector<std::string> OptionalStringList(const YAML::Node& node) {
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

// Reads Phase-1 selection fields. Keys we do not understand (other modifiers,
// fields, lists) are skipped so unsupported Sigma stays forward-compatible.
void ReadSelection(const YAML::Node& selection,
                   std::string& processName,
                   std::string& commandLineContains) {
    if (!selection || !selection.IsMap()) {
        return;
    }

    for (const auto& entry : selection) {
        if (!entry.first.IsScalar()) {
            continue;
        }
        const std::string key = entry.first.as<std::string>();
        if (!entry.second.IsScalar()) {
            continue;
        }
        const std::string value = entry.second.as<std::string>();

        if (key == "process_name") {
            processName = value;
        } else if (key == "command_line|contains" || key == "command_line") {
            // Exact command_line equality is treated as contains for Phase 1
            // so simple Sigma examples still map onto the internal model.
            commandLineContains = value;
        }
        // All other keys/modifiers are intentionally ignored.
    }
}

SigmaRule EmptySigmaRule() {
    return SigmaRule("", "", "", "", "", "", "", "", "", {});
}

}  // namespace

SigmaParseResult SigmaParseResult::Success(SigmaRule rule) {
    const std::string identifier = rule.Title().empty() ? "unknown" : rule.Title();
    return SigmaParseResult(true, std::move(rule), identifier, {});
}

SigmaParseResult SigmaParseResult::Failure(std::string identifier,
                                           std::vector<std::string> errors) {
    return SigmaParseResult(false, EmptySigmaRule(), std::move(identifier), std::move(errors));
}

SigmaParseResult::SigmaParseResult(bool valid,
                                   SigmaRule rule,
                                   std::string identifier,
                                   std::vector<std::string> errors)
    : valid_(valid),
      rule_(std::move(rule)),
      identifier_(std::move(identifier)),
      errors_(std::move(errors)) {}

bool SigmaParseResult::IsValid() const { return valid_; }
const SigmaRule& SigmaParseResult::Rule() const { return rule_; }
const std::string& SigmaParseResult::Identifier() const { return identifier_; }
const std::vector<std::string>& SigmaParseResult::Errors() const { return errors_; }

SigmaParseResult SigmaParser::ParseFile(const std::filesystem::path& path) const {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return SigmaParseResult::Failure(path.filename().string(),
                                         {"Failed to open Sigma rule file: " + path.string()});
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return ParseString(buffer.str(), path.filename().string());
}

SigmaParseResult SigmaParser::ParseString(const std::string& yamlText,
                                          const std::string& identifier) const {
    YAML::Node root;
    try {
        root = YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        return SigmaParseResult::Failure(identifier,
                                         {std::string("Malformed Sigma YAML: ") + e.what()});
    }

    if (!root || !root.IsMap()) {
        return SigmaParseResult::Failure(identifier, {"Sigma root must be a mapping"});
    }

    std::vector<std::string> errors;

    if (!root["title"] || !root["title"].IsScalar() || root["title"].as<std::string>().empty()) {
        errors.push_back("Missing required field: title");
    }
    if (!root["detection"] || !root["detection"].IsMap()) {
        errors.push_back("Missing required field: detection");
    }

    const YAML::Node detection = root["detection"];
    if (detection && detection.IsMap()) {
        if (!detection["selection"] || !detection["selection"].IsMap()) {
            errors.push_back("Missing required field: selection");
        }
        if (!detection["condition"] || !detection["condition"].IsScalar() ||
            detection["condition"].as<std::string>().empty()) {
            errors.push_back("Missing required field: condition");
        } else if (detection["condition"].as<std::string>() != "selection") {
            errors.push_back("Unsupported condition '" + detection["condition"].as<std::string>() +
                             "' (Phase 1 supports only 'selection')");
        }
    }

    const std::string title = OptionalMapString(root, "title");
    const std::string displayName = title.empty() ? identifier : title;

    if (!errors.empty()) {
        return SigmaParseResult::Failure(displayName, std::move(errors));
    }

    std::string processName;
    std::string commandLineContains;
    ReadSelection(detection["selection"], processName, commandLineContains);

    std::string logsourceCategory;
    if (root["logsource"] && root["logsource"].IsMap()) {
        logsourceCategory = OptionalMapString(root["logsource"], "category");
    }

    SigmaRule rule(title, OptionalMapString(root, "id"), OptionalMapString(root, "status"),
                   OptionalMapString(root, "description"), std::move(logsourceCategory),
                   std::move(processName), std::move(commandLineContains),
                   ScalarOrEmpty(detection["condition"]), OptionalMapString(root, "level"),
                   OptionalStringList(root["tags"]));

    return SigmaParseResult::Success(std::move(rule));
}

}  // namespace sentinelforge
