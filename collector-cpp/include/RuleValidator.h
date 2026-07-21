#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Rule.h"

namespace sentinelforge {

// Outcome of validating one rule: whether it passed, and every reason it
// failed if it did not (a rule can fail more than one check at once).
class RuleValidationResult {
public:
    RuleValidationResult(bool valid, std::vector<std::string> errors);

    bool IsValid() const;
    const std::vector<std::string>& Errors() const;

private:
    bool valid_;
    std::vector<std::string> errors_;
};

// Validates a single Rule's required fields, allowed severities, and
// condition block, plus duplicate-name detection against rule names already
// accepted in the current load. Pure validation: no logging, no printing,
// no file access.
class RuleValidator {
public:
    RuleValidationResult Validate(const Rule& rule,
                                   const std::unordered_set<std::string>& existingRuleNames) const;
};

}  // namespace sentinelforge
