// Detection regression harness (Issue #037). Feeds each scenario's events
// through the real collector pipeline in-process (EventNormalizer ->
// DetectionEngine -> CorrelationEngine, via collector_core directly, no
// shelling out to the collector binary) and diffs what fired against what
// the scenario declares expected. See docs/architecture/regression-testing.md.

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "CorrelationEngine.h"
#include "DetectionEngine.h"
#include "EventNormalizer.h"
#include "Logger.h"
#include "Rule.h"
#include "RuleLoader.h"
#include "Scenario.h"
#include "SigmaLoader.h"

namespace {

using sentinelforge::CorrelationEngine;
using sentinelforge::DetectionEngine;
using sentinelforge::DetectionResult;
using sentinelforge::EventNormalizer;
using sentinelforge::Logger;
using sentinelforge::LoggingSettings;
using sentinelforge::LogLevel;
using sentinelforge::Rule;
using sentinelforge::RuleLoader;
using sentinelforge::Scenario;
using sentinelforge::ScenarioLoader;
using sentinelforge::SigmaLoader;

struct ScenarioOutcome {
    std::string name;
    bool passed = true;
    std::vector<std::string> missing;   // expected but did not fire — always a failure
    std::vector<std::string> unexpected;  // fired but not declared expected
};

std::vector<Rule> LoadAllRules(const std::filesystem::path& rulesDir,
                               const std::filesystem::path& sigmaDir, const Logger& logger) {
    RuleLoader ruleLoader;
    const auto discovered = ruleLoader.DiscoverAndParse(rulesDir);
    const auto native = ruleLoader.ValidateRules(discovered);

    std::vector<Rule> rules = native.Accepted();
    for (const auto& rejection : native.Rejected()) {
        std::cerr << "[rule-load] rejected " << rejection.Identifier() << ": ";
        for (const auto& err : rejection.Errors()) {
            std::cerr << err << "; ";
        }
        std::cerr << "\n";
    }

    std::unordered_set<std::string> existingNames;
    for (const auto& rule : rules) {
        existingNames.insert(rule.RuleName());
    }

    SigmaLoader sigmaLoader;
    const auto sigma = sigmaLoader.LoadDirectory(sigmaDir, existingNames, logger);
    for (const auto& rule : sigma.Accepted()) {
        rules.push_back(rule);
    }
    for (const auto& rejection : sigma.Rejected()) {
        std::cerr << "[rule-load] rejected " << rejection.Identifier() << ": ";
        for (const auto& err : rejection.Errors()) {
            std::cerr << err << "; ";
        }
        std::cerr << "\n";
    }

    return rules;
}

// Runs one scenario's events through the pipeline and returns every
// identity that fired: native Rule::RuleName() for matched detections,
// CorrelationAlert::Title() for correlation alerts. A fresh CorrelationEngine
// per scenario keeps scenarios independent of run order.
std::set<std::string> RunScenario(const Scenario& scenario, const std::vector<Rule>& rules) {
    EventNormalizer normalizer;
    DetectionEngine detectionEngine;
    CorrelationEngine correlationEngine;

    std::set<std::string> fired;

    for (const auto& eventJson : scenario.Events()) {
        const auto event = normalizer.NormalizeJson(eventJson);
        const std::vector<DetectionResult> results = detectionEngine.Evaluate(event, rules);
        for (const auto& result : results) {
            if (result.Matched()) {
                fired.insert(result.RuleName());
            }
        }
        const auto alerts = correlationEngine.Process(event, results);
        for (const auto& alert : alerts) {
            fired.insert(alert.Title());
        }
    }

    return fired;
}

ScenarioOutcome Diff(const Scenario& scenario, const std::set<std::string>& fired, bool strict) {
    ScenarioOutcome outcome;
    outcome.name = scenario.Name();

    const std::set<std::string> expected(scenario.ExpectedDetections().begin(),
                                         scenario.ExpectedDetections().end());

    for (const auto& name : expected) {
        if (fired.count(name) == 0) {
            outcome.missing.push_back(name);
        }
    }
    for (const auto& name : fired) {
        if (expected.count(name) == 0) {
            outcome.unexpected.push_back(name);
        }
    }

    outcome.passed = outcome.missing.empty() && (!strict || outcome.unexpected.empty());
    return outcome;
}

void PrintOutcome(const ScenarioOutcome& outcome) {
    std::cout << (outcome.passed ? "[PASS] " : "[FAIL] ") << outcome.name << "\n";
    for (const auto& name : outcome.missing) {
        std::cout << "    MISSING (expected, did not fire): " << name << "\n";
    }
    for (const auto& name : outcome.unexpected) {
        std::cout << "    UNEXPECTED (fired, not declared): " << name << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path scenariosDir = DEFAULT_SCENARIOS_DIR;
    std::filesystem::path rulesDir = DEFAULT_RULES_DIR;
    std::filesystem::path sigmaDir = DEFAULT_SIGMA_RULES_DIR;
    bool strict = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--strict") {
            strict = true;
        } else if (arg == "--scenarios" && i + 1 < argc) {
            scenariosDir = argv[++i];
        } else if (arg == "--rules" && i + 1 < argc) {
            rulesDir = argv[++i];
        } else if (arg == "--sigma-rules" && i + 1 < argc) {
            sigmaDir = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 2;
        }
    }

    Logger logger(LoggingSettings{LogLevel::Warn, /*console=*/true, /*file=*/false, {}});

    const std::vector<Rule> rules = LoadAllRules(rulesDir, sigmaDir, logger);
    std::cout << "Loaded " << rules.size() << " rules from " << rulesDir << " and " << sigmaDir
              << "\n";

    ScenarioLoader scenarioLoader;
    const auto scenarioFiles = scenarioLoader.DiscoverFiles(scenariosDir);
    if (scenarioFiles.empty()) {
        std::cerr << "No scenario files found under " << scenariosDir << "\n";
        return 2;
    }

    std::cout << "Running " << scenarioFiles.size() << " scenario(s) from " << scenariosDir
              << (strict ? " (--strict: unexpected detections fail too)" : "") << "\n\n";

    int failures = 0;
    for (const auto& path : scenarioFiles) {
        const auto loadResult = scenarioLoader.LoadFile(path);
        if (!loadResult.IsValid()) {
            std::cout << "[FAIL] " << loadResult.Identifier() << " (invalid scenario file)\n";
            for (const auto& err : loadResult.Errors()) {
                std::cout << "    " << err << "\n";
            }
            ++failures;
            continue;
        }

        const Scenario& scenario = loadResult.GetScenario();
        const std::set<std::string> fired = RunScenario(scenario, rules);
        const ScenarioOutcome outcome = Diff(scenario, fired, strict);
        PrintOutcome(outcome);
        if (!outcome.passed) {
            ++failures;
        }
    }

    std::cout << "\n"
              << (scenarioFiles.size() - static_cast<std::size_t>(failures)) << "/"
              << scenarioFiles.size() << " scenarios passed\n";

    return failures > 0 ? 1 : 0;
}
