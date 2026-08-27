#include "evaluator.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}
} // namespace

// ==========================================
// KeywordEvaluator Implementation
// ==========================================
KeywordEvaluator::KeywordEvaluator(std::vector<std::string> keywords,
                                   bool match_all,
                                   std::string name,
                                   bool search_full_trajectory)
    : required_keywords(std::move(keywords)),
      match_all(match_all),
      eval_name(std::move(name)),
      search_full_trajectory(search_full_trajectory) {}

std::string KeywordEvaluator::name() const {
    return eval_name;
}

EvalResult KeywordEvaluator::evaluate(const TrajectoryRecord& trajectory) const {
    std::string haystack = toLower(trajectory.final_result);
    if (search_full_trajectory) {
        for (const auto& step : trajectory.steps) {
            haystack += " " + toLower(step.thought);
            haystack += " " + toLower(step.tool_name);
            haystack += " " + toLower(step.tool_args);
            haystack += " " + toLower(step.tool_results);
        }
    }

    std::vector<std::string> found;
    std::vector<std::string> missing;
    for (const auto& kw : required_keywords) {
        if (haystack.find(toLower(kw)) != std::string::npos) {
            found.push_back(kw);
        } else {
            missing.push_back(kw);
        }
    }

    bool passed = match_all ? missing.empty() : !found.empty();
    if (required_keywords.empty()) passed = true;

    std::ostringstream details;
    details << (match_all ? "Required ALL of: " : "Required ANY of: ");
    for (size_t i = 0; i < required_keywords.size(); ++i) {
        details << required_keywords[i];
        if (i + 1 < required_keywords.size()) details << ", ";
    }
    details << " | found: " << found.size() << "/" << required_keywords.size();
    if (!missing.empty()) {
        details << " | missing: ";
        for (size_t i = 0; i < missing.size(); ++i) {
            details << missing[i];
            if (i + 1 < missing.size()) details << ", ";
        }
    }

    return EvalResult{eval_name, passed, details.str()};
}

// ==========================================
// FunctionalEvaluator Implementation
// ==========================================
FunctionalEvaluator::FunctionalEvaluator(std::string name,
                                         std::function<bool(const TrajectoryRecord&)> fn)
    : eval_name(std::move(name)), check_fn(std::move(fn)) {}

std::string FunctionalEvaluator::name() const {
    return eval_name;
}

EvalResult FunctionalEvaluator::evaluate(const TrajectoryRecord& trajectory) const {
    if (!check_fn) {
        return EvalResult{eval_name, false, "no check function provided"};
    }
    bool passed = false;
    std::string details;
    try {
        passed = check_fn(trajectory);
        details = passed ? "check function returned true" : "check function returned false";
    } catch (const std::exception& ex) {
        passed = false;
        details = std::string("check function threw: ") + ex.what();
    }
    return EvalResult{eval_name, passed, details};
}
