#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "../agent/agent_types.h"

// Telemetry record for an individual agent execution step
struct TrajectoryStepRecord {
    int step_id = 0;
    std::string type;         // "thought", "tool_call", "final_answer", "no_action"
    std::string thought;      // LLM thought process
    std::string tool_name;    // Executed tool name if any
    std::string tool_args;    // Tool arguments
    std::string tool_results; // Execution results from tool
    double latency_ms = 0.0;  // Step execution time in milliseconds
    int tokens = 0;           // Estimated or recorded token count for step
};

// Full recorded trajectory of a single agent run
struct TrajectoryRecord {
    std::string task_id;
    std::string model;
    std::string final_result;
    bool success = false;
    double total_latency_ms = 0.0;
    int total_tokens = 0;
    std::vector<TrajectoryStepRecord> steps;
};

// Heuristic token estimation fallback if tokens were not reported by LLM response
inline int estimateTextTokens(const std::string& text) {
    if (text.empty()) return 0;
    std::istringstream iss(text);
    int words = 0;
    std::string w;
    while (iss >> w) ++words;
    return static_cast<int>(words * 1.3) + (words > 0 ? 1 : 0);
}

// Helper function to map agent::Trajectory to TrajectoryRecord
inline TrajectoryRecord mapAgentTrajectory(const agent::Trajectory& agent_traj) {
    TrajectoryRecord rec;
    rec.task_id = agent_traj.task_id;
    rec.model = agent_traj.model;
    rec.final_result = agent_traj.final_answer;
    rec.success = agent_traj.success;
    rec.total_tokens = agent_traj.total_tokens;
    rec.total_latency_ms = 0.0;

    for (const auto& s : agent_traj.steps) {
        TrajectoryStepRecord step_rec;
        step_rec.step_id = s.step_id;
        step_rec.type = s.action_type;
        step_rec.thought = s.thought;
        step_rec.tool_name = s.tool_name;
        step_rec.tool_args = s.tool_args;
        step_rec.tool_results = s.tool_results;
        step_rec.latency_ms = s.latency_ms;
        step_rec.tokens = (s.token_used > 0) ? s.token_used : (estimateTextTokens(s.thought) + estimateTextTokens(s.tool_results));

        rec.total_tokens += step_rec.tokens;
        rec.total_latency_ms += s.latency_ms;
        rec.steps.push_back(step_rec);
    }
    return rec;
}
