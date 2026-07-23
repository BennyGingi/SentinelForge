#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "SigmaRule.h"

namespace sentinelforge {

// Outcome of parsing and schema-validating one Sigma YAML file.
class SigmaParseResult {
public:
    static SigmaParseResult Success(SigmaRule rule);
    static SigmaParseResult Failure(std::string identifier, std::vector<std::string> errors);

    bool IsValid() const;
    const SigmaRule& Rule() const;
    const std::string& Identifier() const;
    const std::vector<std::string>& Errors() const;

private:
    SigmaParseResult(bool valid,
                     SigmaRule rule,
                     std::string identifier,
                     std::vector<std::string> errors);

    bool valid_;
    SigmaRule rule_;
    std::string identifier_;
    std::vector<std::string> errors_;
};

// Parses Sigma YAML into a SigmaRule and validates the Phase-1 supported
// schema (title, detection, selection, condition). Unsupported fields are
// ignored. Does not translate into the internal Rule model.
class SigmaParser {
public:
    SigmaParseResult ParseFile(const std::filesystem::path& path) const;
    SigmaParseResult ParseString(const std::string& yamlText,
                                 const std::string& identifier) const;
};

}  // namespace sentinelforge
