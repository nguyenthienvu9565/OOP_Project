#include "harness.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

using json = nlohmann::json;

HarnessRunner::HarnessRunner(std::shared_ptr<agent::AgentLoop> agent)
    : agent(std::move(agent)) {}

void HarnessRunner::addEvaluator(std::shared_ptr<Evaluator> evaluator) {
    evaluators.push_back(std::move(evaluator));
}

void HarnessRunner::clearEvaluators() {
    evaluators.clear();
}

void HarnessRunner::setupEnvironment() {
    environment_ready = true;
}

TaskResult HarnessRunner::runTask(const Task& task) {
    if (!environment_ready) {
        setupEnvironment();
    }

    // Run real AgentLoop from Tầng 4 & 5
    agent::Trajectory agent_traj = agent->run(task.id, task.prompt);

    // Map to TrajectoryRecord telemetry format
    TrajectoryRecord trajectory = mapAgentTrajectory(agent_traj);

    // Evaluate trajectory against registered evaluators
    std::vector<EvalResult> evals;
    evals.reserve(evaluators.size() + (task.keywords.empty() ? 0 : 1));

    bool all_passed = true;

    // Task-level KeywordEvaluator if task.keywords provided
    if (!task.keywords.empty()) {
        KeywordEvaluator task_kw_eval(task.keywords, true, "TaskKeywordEvaluator");
        EvalResult r = task_kw_eval.evaluate(trajectory);
        all_passed = all_passed && r.passed;
        evals.push_back(std::move(r));
    }

    for (const auto& ev : evaluators) {
        EvalResult r = ev->evaluate(trajectory);
        all_passed = all_passed && r.passed;
        evals.push_back(std::move(r));
    }

    TaskResult result;
    result.task_id = task.id;
    result.trajectory = std::move(trajectory);
    result.evaluations = std::move(evals);
    result.success = all_passed;

    recorded_results.push_back(result);
    return result;
}

std::vector<TaskResult> HarnessRunner::runBatch(const std::vector<Task>& tasks) {
    std::vector<TaskResult> results;
    results.reserve(tasks.size());
    for (const auto& task : tasks) {
        results.push_back(runTask(task));
    }
    return results;
}

double HarnessRunner::calculateSuccessRate(const std::vector<TaskResult>& results) const {
    if (results.empty()) return 0.0;
    size_t passed = std::count_if(results.begin(), results.end(),
                                   [](const TaskResult& r) { return r.success; });
    return static_cast<double>(passed) / static_cast<double>(results.size());
}

const std::vector<TaskResult>& HarnessRunner::getRecordedResults() const {
    return recorded_results;
}

void HarnessRunner::exportToJson(const std::vector<TaskResult>& results, const std::string& filepath) const {
    json j_root;
    j_root["success_rate"] = calculateSuccessRate(results);
    j_root["total_tasks"] = results.size();

    size_t passed_count = std::count_if(results.begin(), results.end(), [](const TaskResult& r) { return r.success; });
    j_root["passed_tasks"] = passed_count;

    json j_results = json::array();

    for (const auto& r : results) {
        json j_res;
        j_res["task_id"] = r.task_id;
        j_res["success"] = r.success;

        json j_traj;
        j_traj["final_result"] = r.trajectory.final_result;
        j_traj["total_latency_ms"] = r.trajectory.total_latency_ms;
        j_traj["total_tokens"] = r.trajectory.total_tokens;

        json j_steps = json::array();
        for (const auto& step : r.trajectory.steps) {
            json j_s;
            j_s["step_id"] = step.step_id;
            j_s["type"] = step.type;
            j_s["thought"] = step.thought;
            j_s["tool_name"] = step.tool_name;
            j_s["tool_args"] = step.tool_args;
            j_s["tool_results"] = step.tool_results;
            j_s["latency_ms"] = step.latency_ms;
            j_s["tokens"] = step.tokens;
            j_steps.push_back(j_s);
        }
        j_traj["steps"] = j_steps;
        j_res["trajectory"] = j_traj;

        json j_evals = json::array();
        for (const auto& ev : r.evaluations) {
            json j_e;
            j_e["evaluator"] = ev.evaluator_name;
            j_e["passed"] = ev.passed;
            j_e["details"] = ev.details;
            j_evals.push_back(j_e);
        }
        j_res["evaluations"] = j_evals;

        j_results.push_back(j_res);
    }

    j_root["results"] = j_results;

    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing JSON: " + filepath);
    }
    file << j_root.dump(4);
}

std::vector<Task> HarnessRunner::loadTasksFromJson(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open tasks JSON file: " + filepath);
    }
    json j;
    file >> j;

    std::vector<Task> tasks;
    if (j.is_array()) {
        for (const auto& item : j) {
            Task t;
            t.id = item.value("id", "");
            t.prompt = item.value("prompt", "");
            if (item.contains("keywords") && item["keywords"].is_array()) {
                for (const auto& kw : item["keywords"]) {
                    t.keywords.push_back(kw.get<std::string>());
                }
            }
            tasks.push_back(t);
        }
    }
    return tasks;
}
