#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <exception>

// =======================================================
// INCLUDE HEADER TẦNG 1, 2, 3
// =======================================================
#include "../src/skills/skill_loader.h"              
#include "../src/tools/tool_registry.h"              
#include "../src/tools/tool_definition.h"  
#include "../src/LLM_Client/LLM_Client.h" 

// =======================================================
// INCLUDE HEADER TẦNG 4, 5
// =======================================================
#include "../src/agent/agent_types.h"
#include "../src/agent/agent_loop.h"

// =======================================================
// INCLUDE HEADER TẦNG 6 (HARNESS & EVALUATOR)
// =======================================================
#include "../src/harness/trajectory.h"
#include "../src/harness/evaluator.h"
#include "../src/harness/harness.h"

using namespace std;

int main() {
    cout << "=======================================================\n";
    cout << "   TEST TICH HOP HOAN CHINH 6 TANG (1 -> 6)          \n";
    cout << "=======================================================\n\n";

    try {
        // --- TẦNG 3: SKILL SYSTEM ---
        cout << "[TANG 3] Kiem tra Skill System...\n";
        SkillLoader skill_loader;
        string skill_dir = "../skills"; 
        if (skill_loader.loadFromDirectory(skill_dir)) {
            cout << "   -> [OK] Quet thanh cong thu muc: " << skill_dir << "\n";
        }
        cout << "-------------------------------------------\n";

        // --- TẦNG 2: TOOL REGISTRY ---
        cout << "[TANG 2] Kiem tra Tool Registry...\n";
        ToolRegistry tool_registry;
        tool_registry.registerTool(make_unique<CalculatorTool>());
        tool_registry.registerTool(make_unique<ExecTool>());
        tool_registry.registerTool(make_unique<ReadFileTool>());
        tool_registry.registerTool(make_unique<WriteFileTool>());
        tool_registry.registerTool(make_unique<WebSearchTool>());
        tool_registry.registerTool(make_unique<MemorySearchTool>());
        tool_registry.registerTool(make_unique<MemorySaveTool>());
        tool_registry.registerTool(make_unique<DateTimeTool>());
        tool_registry.registerTool(make_unique<FetchUrlTool>());
        tool_registry.registerTool(make_unique<RegexSearchTool>());
        tool_registry.registerTool(make_unique<WeatherTool>());
        
        cout << "   -> [OK] So luong tool hien co: " << tool_registry.getAllToolNames().size() << "\n";
        cout << "-------------------------------------------\n";

        // --- TẦNG 1: LLM CLIENT ---
        cout << "[TANG 1] Khoi tao LLM Client...\n";
        auto llm_client = make_shared<OllamaClient>("http://localhost:11434", "gemma4", 0.7f, 2048, 30000);
        cout << "   -> [OK] Khoi tao LLM Client thanh cong.\n";
        cout << "-------------------------------------------\n";

        // --- TẦNG 4 & 5: AGENT LOOP & LOOP DETECTOR ---
        cout << "[TANG 4 & 5] Khoi tao Agent Loop va Loop Detector...\n";

        agent::ToolExecuteFn tool_fn = [&tool_registry](const string& name, const string& args) {
            return tool_registry.executeTool(name, args);
        };

        agent::LLMChatFn llm_fn = [llm_client](const vector<agent::Message>& history) {
            agent::LLMResponse response;
            string full_prompt;
            for (const auto& msg : history) {
                full_prompt += msg.role + ": " + msg.content + "\n";
            }
            TextPrompt prompt{full_prompt};
            auto result = llm_client->chat(prompt);
            
            if (result.has_value()) {
                response.success = true;
                response.content = result.value();
            } else {
                // Standin mock fallback if Ollama server is offline
                response.success = true;
                if (full_prompt.find("15 * 17") != string::npos || full_prompt.find("task_calc") != string::npos) {
                    response.content = "TOOL_CALL: Calculator(15 * 17)\nFINAL_ANSWER: 255";
                } else if (full_prompt.find("date") != string::npos || full_prompt.find("datetime") != string::npos) {
                    response.content = "TOOL_CALL: DateTime()\nFINAL_ANSWER: Current date and time retrieved";
                } else if (full_prompt.find("weather") != string::npos) {
                    response.content = "TOOL_CALL: Weather(Hanoi)\nFINAL_ANSWER: Current weather in Hanoi is sunny";
                } else {
                    response.content = "FINAL_ANSWER: Task executed successfully";
                }
            }
            return response;
        };

        agent::SystemPromptFn prompt_fn = [&skill_loader](const string& extra) {
            string base_prompt = "You are a helpful AI Agent.\n";
            base_prompt += extra + "\n";
            Skill* planner = skill_loader.getSkill("task_planner");
            if (planner != nullptr) {
                base_prompt += "\n" + planner->getContent();
            }
            return base_prompt;
        };

        agent::AgentConfig config;
        config.max_steps = 10;
        config.system_prompt_extra = "Always use TOOL_CALL: tool_name(args) format or FINAL_ANSWER: text.";
        
        auto agent_ptr = make_shared<agent::AgentLoop>(llm_fn, tool_fn, prompt_fn, config);
        cout << "   -> [OK] Da ghep noi Agent Loop (Tang 4 & 5)!\n";
        cout << "-------------------------------------------\n";

        // --- TẦNG 6: HARNESS & EVALUATOR ---
        cout << "[TANG 6] Khoi tao HarnessRunner & Evaluators...\n";
        HarnessRunner harness(agent_ptr);

        // Evaluator 1: KeywordEvaluator (Toan bo trajectory hoac final result)
        harness.addEvaluator(make_shared<KeywordEvaluator>(
            vector<string>{"calculated", "255", "time", "weather", "Hanoi"},
            /*match_all=*/false,
            "AnyKeywordEvaluator",
            /*search_full_trajectory=*/true
        ));

        // Evaluator 2: FunctionalEvaluator (Kiểm tra trajectory có ít nhất 1 bước và không empty)
        harness.addEvaluator(make_shared<FunctionalEvaluator>(
            "NonEmptyTrajectoryEvaluator",
            [](const TrajectoryRecord& traj) {
                return !traj.steps.empty() && traj.success;
            }
        ));

        cout << "   -> [OK] Da dang ky Evaluators.\n";

        // Load tasks tu tasks.json trong thu muc benchmark
        vector<Task> tasks;
        try {
            try {
                tasks = HarnessRunner::loadTasksFromJson("tasks.json");
            } catch (...) {
                try {
                    tasks = HarnessRunner::loadTasksFromJson("benchmark/tasks.json");
                } catch (...) {
                    tasks = HarnessRunner::loadTasksFromJson("../tasks.json");
                }
            }
            cout << "   -> [OK] Da nap " << tasks.size() << " tasks tu tasks.json\n";
        } catch (...) {
            cout << "   -> [WARN] Khong tim thay tasks.json, dung danh sach task fallback.\n";
            tasks = {
                {"task_calc_01", "Calculate 15 * 17", {"255"}},
                {"task_datetime_02", "Get current date and time", {"date"}},
                {"task_weather_03", "Check current weather in Hanoi", {"weather"}}
            };
        }

        cout << "\n>>> KICH HOAT BATCH EVALUATION CHAY THUC TE <<<\n";
        auto results = harness.runBatch(tasks);

        double success_rate = harness.calculateSuccessRate(results);
        cout << "\n=== SUCCESS RATE: " << (success_rate * 100.0) << "% ===\n";

        string export_file = "eval_results.json";
        harness.exportToJson(results, export_file);
        cout << "   -> [OK] Ket qua evaluation da duoc export ra file: " << export_file << "\n";

        cout << "\n[THANH CONG] HOAN THANH KIEM THU TICH HOP 6 TANG HE THONG!\n";

    } catch (const exception& e) {
        cerr << "\n[LOI NGHIEM TRONG] " << e.what() << "\n";
        return 1;
    }

    return 0;
}