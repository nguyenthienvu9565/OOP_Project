#pragma once
#include "agent_types.h"
#include "loop_detector.h"
#include <vector>
#include <optional>
#include <string>
#include <utility> 

namespace agent
{
    class AgentLoop
    {
    private:
        LLMChatFn llm_fn_;
        ToolExecuteFn tool_fn_;
        SystemPromptFn prompt_fn_;
        AgentConfig cfg_;
        LoopDetector loop_detector_;
        std::vector<Message> history_;
        std::optional<StepHook> step_hook_;

    public:
        AgentLoop(LLMChatFn llm_fn, ToolExecuteFn tool_fn, SystemPromptFn prompt_fn, AgentConfig cfg)
            : llm_fn_(llm_fn), 
              tool_fn_(tool_fn), 
              prompt_fn_(prompt_fn), 
              cfg_(cfg), 
              loop_detector_(cfg.loop_cfg)
        {
        }

        virtual ~AgentLoop() = default;

        [[nodiscard]] virtual Trajectory run(const std::string& task_id, const std::string& user_query);
        
        void set_step_hook(StepHook hook) 
        {
            step_hook_ = std::move(hook); 
        }

    protected:
        virtual LLMResponse observe();
        virtual ParsedAction think(const LLMResponse& response);
        virtual std::string act_on(const ToolCallAction& action);
    };
}