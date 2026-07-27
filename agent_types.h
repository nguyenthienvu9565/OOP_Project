#pragma once
#include <optional>
#include <string>
#include <variant>
#include <functional>
#include <vector>
#include "loop_detector.h"

namespace agent
{
    struct Message
    {
        std::string role;
        std::string content;
        std::optional<std::string> image_base64;
    };

    struct Step {
    int step_id      = 0;
    std::string thought;
    std::string action_type;
    std::string tool_name;
    std::string tool_args;
    std::string tool_results;
    int token_used = 0;
    double latency_ms = 0.0;
    };

    struct Trajectory {
    std::string task_id;
    std::string model;
    bool success = false;  
    std::string final_answer;
    int total_tokens = 0;     
    std::vector<Step> steps;
    void add_step(const Step& step) { steps.push_back(step); }
    };

    struct LLMResponse {
    bool success = false;
    std::string content;
    std::string error_message;
    int prompt_tokens = 0;
    int completion_tokens = 0;
};
    
    struct ToolCallAction 
    {
        std::string tool_name;
        std::string tool_args;
    };

    struct FinalAnswerAction
    {
        std::string answer;
    };

    struct NoAction
    {
        std::string reason;
    };

    using ParsedAction = std::variant<ToolCallAction, FinalAnswerAction, NoAction>;
    using LLMChatFn = std::function<LLMResponse(const std::vector<Message>&)>;
    using ToolExecuteFn = std::function<std::string(const std::string&, const std::string&)>;
    using StepHook = std::function<void(const Step&)>;
    using SystemPromptFn = std::function<std::string(const std::string&)>;

    struct AgentConfig 
    {
        int max_steps = 10;
        bool verbose = false;
        std::string system_prompt_extra;
        LoopDetectorConfig loop_cfg;
    };
}