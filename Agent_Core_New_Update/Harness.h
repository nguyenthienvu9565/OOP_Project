#pragma once
#include "Agent_Core_New_Update.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

// ==========================================
// 3.6 HARNESS & EVALUATOR
// ==========================================

// Full recorded trajectory of a single agent run: every instrumented step
// (thought/action/observation, each with latency + estimated tokens) plus
// the final result string returned by AgentLoop::run().
struct TrajectoryRecord {
    std::vector<TrajectoryStep> steps;
    std::string final_result;
    double total_latency_ms = 0.0;
    int total_tokens = 0;
};

// Result of one evaluator run against one trajectory.
struct EvalResult {
    std::string evaluator_name;
    bool passed = false;
    std::string details;
};

// ------------------------------------------
// Evaluator interface
// ------------------------------------------
class Evaluator {
public:
    virtual ~Evaluator() = default;
    virtual std::string name() const = 0;
    virtual EvalResult evaluate(const TrajectoryRecord& trajectory) const = 0;
};

// Passes if the final result contains the required keyword(s).
// match_all = true  -> ALL keywords must be present (AND)
// match_all = false -> AT LEAST ONE keyword must be present (OR)
class KeywordEvaluator : public Evaluator {
    std::vector<std::string> required_keywords;
    bool match_all;
    std::string eval_name;

public:
    explicit KeywordEvaluator(std::vector<std::string> keywords,
                               bool match_all = true,
                               std::string name = "KeywordEvaluator");

    std::string name() const override;
    EvalResult evaluate(const TrajectoryRecord& trajectory) const override;
};

// Passes if a user-supplied predicate over the full trajectory returns true.
// This is the escape hatch for checks that keyword-matching can't express
// (e.g. "did the agent call at least 1 action", "latency under budget", etc).
class FunctionalEvaluator : public Evaluator {
    std::string eval_name;
    std::function<bool(const TrajectoryRecord&)> check_fn;

public:
    FunctionalEvaluator(std::string name, std::function<bool(const TrajectoryRecord&)> fn);

    std::string name() const override;
    EvalResult evaluate(const TrajectoryRecord& trajectory) const override;
};

// ------------------------------------------
// Task / TaskResult
// ------------------------------------------
struct Task {
    std::string id;
    std::string prompt;
};

struct TaskResult {
    std::string task_id;
    TrajectoryRecord trajectory;
    std::vector<EvalResult> evaluations;
    bool success = false; // true iff every attached evaluator passed
};

// ------------------------------------------
// HarnessRunner: setup -> run -> evaluate -> record
// ------------------------------------------
class HarnessRunner {
    std::shared_ptr<AgentLoop> agent;
    std::vector<std::shared_ptr<Evaluator>> evaluators;
    std::vector<TaskResult> recorded_results;

public:
    explicit HarnessRunner(std::shared_ptr<AgentLoop> agent);

    void addEvaluator(std::shared_ptr<Evaluator> evaluator);

    // Environment setup hook point (attaches the trajectory recorder to the
    // agent). Safe to call multiple times; called automatically by runTask
    // if not called already.
    void setupEnvironment();

    // Runs a single task end-to-end: setup -> run agent -> evaluate -> record.
    TaskResult runTask(const Task& task);

    // Runs a set of tasks and returns all results (also stored internally
    // for later export / success-rate calculation).
    std::vector<TaskResult> runBatch(const std::vector<Task>& tasks);

    // success rate in [0.0, 1.0] over the given results.
    double calculateSuccessRate(const std::vector<TaskResult>& results) const;

    // All results recorded so far via runTask/runBatch.
    const std::vector<TaskResult>& getRecordedResults() const;

    // Serializes results (trajectory + evaluations + success) to a JSON file.
    // Hand-rolled writer, no external JSON dependency required.
    void exportToJson(const std::vector<TaskResult>& results, const std::string& filepath) const;

private:
    bool environment_ready = false;
};