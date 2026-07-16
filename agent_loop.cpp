#include "agent_loop.h"
#include <regex>
#include <chrono> 

namespace agent
{
    LLMResponse AgentLoop::observe()
    {
        return llm_fn_(history_);
    }

    std::string AgentLoop::act_on(const ToolCallAction& action)
    {
        return tool_fn_(action.tool_name, action.tool_args);
    }

    ParsedAction AgentLoop::think(const LLMResponse& response)
    {
        if (!response.success)
        {
            NoAction action;
            action.reason = response.error_message;
            return action;
        }

        std::regex final_answer_pattern(R"(FINAL_ANSWER:\s*(.*))");
        std::regex tool_call_pattern(R"(TOOL_CALL:\s*([a-zA-Z0-9_]+)\((.*)\))");
        std::smatch matches;

        if (std::regex_search(response.content, matches, final_answer_pattern))
        {
            FinalAnswerAction final_act;
            final_act.answer = matches[1].str();
            return final_act;
        }

        if (std::regex_search(response.content, matches, tool_call_pattern))
        {
            ToolCallAction tool_act;
            tool_act.tool_name = matches[1].str();
            tool_act.tool_args = matches[2].str();
            return tool_act;
        }

        return NoAction{"LLM response does not match any required format."};
    }

    Trajectory AgentLoop::run(const std::string& task_id, const std::string& user_query)
    {
        Trajectory traj;
        traj.task_id = task_id;
        history_.clear();
        loop_detector_.reset(); 

        std::string sys_content = prompt_fn_(cfg_.system_prompt_extra);
        history_.push_back(Message{"system", sys_content, std::nullopt});
        history_.push_back(Message{"user", user_query, std::nullopt});

        for (int step_id = 0; step_id < cfg_.max_steps; ++step_id)
        {
            auto step_start = std::chrono::steady_clock::now();

            Step current_step;
            current_step.step_id = step_id;

            LLMResponse response = observe();
            current_step.thought = response.content; 
            
            ParsedAction action = think(response);

            if (std::holds_alternative<FinalAnswerAction>(action))
            {
                auto final_ans = std::get<FinalAnswerAction>(action);
                current_step.action_type = "final_answer";
                
                traj.success = true;
                traj.final_answer = final_ans.answer;

                auto step_end = std::chrono::steady_clock::now();
                current_step.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_end - step_start).count();
                
                traj.steps.push_back(current_step);

                if (step_hook_.has_value())
                {
                    (*step_hook_)(current_step);
                }
                break; 
            }    
            
            if (std::holds_alternative<ToolCallAction>(action))
            {
                auto tool_act = std::get<ToolCallAction>(action);
                current_step.action_type = "tool_call";
                current_step.tool_name = tool_act.tool_name;
                current_step.tool_args = tool_act.tool_args;

                std::string signature = tool_act.tool_name + "(" + tool_act.tool_args + ")";
                LoopSeverity sev = loop_detector_.record(signature);

                if (sev == LoopSeverity::Critical)
                {
                    traj.success = false;
                    traj.final_answer = "Loop detected! Reason: " + loop_detector_.last_reason();
                    
                    auto step_end = std::chrono::steady_clock::now();
                    current_step.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_end - step_start).count();
                    
                    traj.steps.push_back(current_step);
                    if (step_hook_.has_value()) (*step_hook_)(current_step);
                    break;
                }

                std::string tool_result = act_on(tool_act);
                current_step.tool_results = tool_result;

                history_.push_back(Message{"assistant", response.content, std::nullopt});
                history_.push_back(Message{"user", 
                    "Tool result: " + tool_result + "\nContinue. If done write FINAL_ANSWER: ...", 
                    std::nullopt});
                
                auto step_end = std::chrono::steady_clock::now();
                current_step.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_end - step_start).count();

                traj.steps.push_back(current_step);

                if (step_hook_.has_value())
                {
                    (*step_hook_)(current_step);
                }
            }

            if (std::holds_alternative<NoAction>(action))
            {
                auto no_act = std::get<NoAction>(action);
                current_step.action_type = "no_action";
                current_step.tool_results = no_act.reason;

                history_.push_back(Message{"user", 
                    "Please use the strict format: TOOL_CALL: name(args) or FINAL_ANSWER: text", 
                    std::nullopt});
                
                auto step_end = std::chrono::steady_clock::now();
                current_step.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_end - step_start).count();

                traj.steps.push_back(current_step);
                
                if (step_hook_.has_value())
                {
                    (*step_hook_)(current_step);
                }
            }
        }

        return traj; 
    }
}