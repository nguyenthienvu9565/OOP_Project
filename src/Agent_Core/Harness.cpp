#include "Harness.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

// ==========================================
// JSON helpers (hand-rolled, no external dependency)
// ==========================================
namespace {

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::string jsonStep(const TrajectoryStep& step, const std::string& indent) {
    std::ostringstream oss;
    oss << indent << "{\n"
        << indent << "  \"type\": \"" << jsonEscape(step.type) << "\",\n"
        << indent << "  \"content\": \"" << jsonEscape(step.content) << "\",\n"
        << indent << "  \"latency_ms\": " << step.latency_ms << ",\n"
        << indent << "  \"tokens\": " << step.tokens << "\n"
        << indent << "}";
    return oss.str();
}

std::string jsonEval(const EvalResult& e, const std::string& indent) {
    std::ostringstream oss;
    oss << indent << "{\n"
        << indent << "  \"evaluator\": \"" << jsonEscape(e.evaluator_name) << "\",\n"
        << indent << "  \"passed\": " << (e.passed ? "true" : "false") << ",\n"
        << indent << "  \"details\": \"" << jsonEscape(e.details) << "\"\n"
        << indent << "}";
    return oss.str();
}

std::string jsonTaskResult(const TaskResult& r, const std::string& indent) {
    std::ostringstream oss;
    oss << indent << "{\n"
        << indent << "  \"task_id\": \"" << jsonEscape(r.task_id) << "\",\n"
        << indent << "  \"success\": " << (r.success ? "true" : "false") << ",\n"
        << indent << "  \"trajectory\": {\n"
        << indent << "    \"final_result\": \"" << jsonEscape(r.trajectory.final_result) << "\",\n"
        << indent << "    \"total_latency_ms\": " << r.trajectory.total_latency_ms << ",\n"
        << indent << "    \"total_tokens\": " << r.trajectory.total_tokens << ",\n"
        << indent << "    \"steps\": [\n";
    for (size_t i = 0; i < r.trajectory.steps.size(); ++i) {
        oss << jsonStep(r.trajectory.steps[i], indent + "      ");
        if (i + 1 < r.trajectory.steps.size()) oss << ",";
        oss << "\n";
    }
    oss << indent << "    ]\n"
        << indent << "  },\n"
        << indent << "  \"evaluations\": [\n";
    for (size_t i = 0; i < r.evaluations.size(); ++i) {
        oss << jsonEval(r.evaluations[i], indent + "    ");
        if (i + 1 < r.evaluations.size()) oss << ",";
        oss << "\n";
    }
    oss << indent << "  ]\n"
        << indent << "}";
    return oss.str();
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // namespace

// ==========================================
// KeywordEvaluator
// ==========================================
KeywordEvaluator::KeywordEvaluator(std::vector<std::string> keywords,
                                   bool match_all,
                                   std::string name)
    : required_keywords(std::move(keywords)),
      match_all(match_all),
      eval_name(std::move(name)) {}

std::string KeywordEvaluator::name() const { return eval_name; }

EvalResult KeywordEvaluator::evaluate(const TrajectoryRecord& trajectory) const {
    // Search the final result. Case-insensitive substring match.
    const std::string haystack = toLower(trajectory.final_result);

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
    if (required_keywords.empty()) passed = true; // nothing required -> trivially passes

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
// FunctionalEvaluator
// ==========================================
FunctionalEvaluator::FunctionalEvaluator(std::string name,
                                          std::function<bool(const TrajectoryRecord&)> fn)
    : eval_name(std::move(name)), check_fn(std::move(fn)) {}

std::string FunctionalEvaluator::name() const { return eval_name; }

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

// ==========================================
// HarnessRunner
// ==========================================
HarnessRunner::HarnessRunner(std::shared_ptr<AgentLoop> agent)
    : agent(std::move(agent)) {}

void HarnessRunner::addEvaluator(std::shared_ptr<Evaluator> evaluator) {
    evaluators.push_back(std::move(evaluator));
}

void HarnessRunner::setupEnvironment() {
    // Placeholder for real environment setup (spinning up sandboxes, seeding
    // mock data stores, resetting tool state, etc). Kept as an explicit step
    // so subclasses/future code have a single place to hook into.
    environment_ready = true;
}

TaskResult HarnessRunner::runTask(const Task& task) {
    if (!environment_ready) {
        setupEnvironment();
    }

    // --- Set up environment / recorder ---
    TrajectoryRecord trajectory;
    agent->setTrajectoryHook([&trajectory](const TrajectoryStep& step) {
        trajectory.steps.push_back(step);
        trajectory.total_latency_ms += step.latency_ms;
        trajectory.total_tokens += step.tokens;
    });

    // --- Run agent ---
    trajectory.final_result = agent->run(task.prompt);

    // --- Evaluate ---
    std::vector<EvalResult> evals;
    evals.reserve(evaluators.size());
    bool all_passed = true;
    for (const auto& ev : evaluators) {
        EvalResult r = ev->evaluate(trajectory);
        all_passed = all_passed && r.passed;
        evals.push_back(std::move(r));
    }

    // --- Record ---
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

void HarnessRunner::exportToJson(const std::vector<TaskResult>& results,
                                  const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out) {
        throw std::runtime_error("Không thể mở file để ghi JSON: " + filepath);
    }

    double success_rate = calculateSuccessRate(results);

    out << "{\n";
    out << "  \"success_rate\": " << success_rate << ",\n";
    out << "  \"total_tasks\": " << results.size() << ",\n";
    out << "  \"results\": [\n";
    for (size_t i = 0; i < results.size(); ++i) {
        out << jsonTaskResult(results[i], "    ");
        if (i + 1 < results.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}