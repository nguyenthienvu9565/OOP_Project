#pragma once
#include "trajectory.h"
#include "evaluator.h"
#include "../agent/agent_loop.h"
#include <vector>
#include <string>
#include <memory>

struct Task {
    std::string id;
    std::string prompt;
    std::vector<std::string> keywords;
};

struct TaskResult {
    std::string task_id;
    TrajectoryRecord trajectory;
    std::vector<EvalResult> evaluations;
    bool success = false;
};

// HarnessRunner: setup environment -> run real agent -> evaluate -> record
class HarnessRunner {
private:
    std::shared_ptr<agent::AgentLoop> agent;
    std::vector<std::shared_ptr<Evaluator>> evaluators;
    std::vector<TaskResult> recorded_results;
    bool environment_ready = false;

public:
    explicit HarnessRunner(std::shared_ptr<agent::AgentLoop> agent);

    void addEvaluator(std::shared_ptr<Evaluator> evaluator);
    void clearEvaluators();

    void setupEnvironment();

    TaskResult runTask(const Task& task);
    std::vector<TaskResult> runBatch(const std::vector<Task>& tasks);

    double calculateSuccessRate(const std::vector<TaskResult>& results) const;
    const std::vector<TaskResult>& getRecordedResults() const;

    void exportToJson(const std::vector<TaskResult>& results, const std::string& filepath) const;
    static std::vector<Task> loadTasksFromJson(const std::string& filepath);
};
