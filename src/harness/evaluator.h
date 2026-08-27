#pragma once
#include "trajectory.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

// Result of one evaluator run against one trajectory.
struct EvalResult {
    std::string evaluator_name;
    bool passed = false;
    std::string details;
};

// ------------------------------------------
// Evaluator Interface
// ------------------------------------------
class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual std::string name() const = 0;
    virtual EvalResult evaluate(const TrajectoryRecord& trajectory) const = 0;
};

// Passes if final result or trajectory content contains the required keyword(s).
// match_all = true  -> ALL keywords must be present (AND)
// match_all = false -> AT LEAST ONE keyword must be present (OR)
class KeywordEvaluator : public Evaluator {
private:
    std::vector<std::string> required_keywords;
    bool match_all;
    std::string eval_name;
    bool search_full_trajectory;

public:
    explicit KeywordEvaluator(std::vector<std::string> keywords,
                             bool match_all = true,
                             std::string name = "KeywordEvaluator",
                             bool search_full_trajectory = true);

    std::string name() const override;
    EvalResult evaluate(const TrajectoryRecord& trajectory) const override;
};

// Passes if a user-supplied predicate over the full trajectory returns true.
class FunctionalEvaluator : public Evaluator {
private:
    std::string eval_name;
    std::function<bool(const TrajectoryRecord&)> check_fn;

public:
    FunctionalEvaluator(std::string name, std::function<bool(const TrajectoryRecord&)> fn);

    std::string name() const override;
    EvalResult evaluate(const TrajectoryRecord& trajectory) const override;
};
