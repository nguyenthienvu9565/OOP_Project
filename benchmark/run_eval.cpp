#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <exception>

// =======================================================
// INCLUDE CÁC FILE HEADER CỦA TẦNG 1, 2, 3
// =======================================================
#include "../src/skills/skill_loader.h"              
#include "../src/tools/tool_registry.h"              
#include "../src/tools/tool_definition.h"  
#include "../src/LLM_Client/LLM_Client.h" 

// =======================================================
// INCLUDE CÁC FILE HEADER CỦA TẦNG 4 & 5
// (Lưu ý sửa lại đường dẫn theo đúng cây thư mục của bạn)
// =======================================================
#include "../src/agent/agent_types.h"
#include "../src/agent/agent_loop.h"

using namespace std;

int main() {
    cout << "===========================================\n";
    cout << "   TEST TICH HOP: 5 TANG (1, 2, 3, 4, 5)   \n";
    cout << "===========================================\n\n";

    try {
        // --- GIỮ NGUYÊN CODE TẦNG 3 ---
        cout << "[TANG 3] Kiem tra Skill System...\n";
        SkillLoader skill_loader;
        string skill_dir = "../skills"; 
        if (skill_loader.loadFromDirectory(skill_dir)) {
            cout << "   -> [OK] Quet thanh cong thu muc: " << skill_dir << "\n";
        }
        cout << "-------------------------------------------\n";

        // --- GIỮ NGUYÊN CODE TẦNG 2 ---
        cout << "[TANG 2] Kiem tra Tool Registry...\n";
        ToolRegistry tool_registry;
        tool_registry.registerTool(make_unique<CalculatorTool>());
        tool_registry.registerTool(make_unique<ExecTool>());
        tool_registry.registerTool(make_unique<ReadFileTool>());
        tool_registry.registerTool(make_unique<WriteFileTool>());
        tool_registry.registerTool(make_unique<WebSearchTool>());
        tool_registry.registerTool(make_unique<MemorySearchTool>());
        tool_registry.registerTool(make_unique<MemorySaveTool>());
        
        cout << "   -> [OK] So luong tool hien co: " << tool_registry.getAllToolNames().size() << "\n";
        cout << "-------------------------------------------\n";

        // --- GIỮ NGUYÊN CODE TẦNG 1 ---
        cout << "[TANG 1] Kiem tra LLM Client (Ollama)...\n";
        OllamaClient llm_client("http://localhost:11434", "gemma4", 0.7f, 2048, 30000);
        cout << "   -> [OK] Khoi tao LLM Client thanh cong.\n";
        cout << "-------------------------------------------\n";

        // =========================================================
        // TẦNG 4 & 5: AGENT LOOP & LOOP DETECTOR
        // Kết nối các module 1, 2, 3 bằng hàm Lambda
        // =========================================================
        cout << "[TANG 4 & 5] Khoi tao Agent Loop va Loop Detector...\n";

        // 1. Tạo callback chạy Tool (Gói Tầng 2)
        agent::ToolExecuteFn tool_fn = [&tool_registry](const string& name, const string& args) {
            return tool_registry.executeTool(name, args);
        };

        // 2. Tạo callback gọi LLM (Gói Tầng 1)
        agent::LLMChatFn llm_fn = [&llm_client](const vector<agent::Message>& history) {
            agent::LLMResponse response;
            
            // Chuyển đổi lịch sử Message sang dạng string thuần để gọi LLMClient
            string full_prompt;
            for (const auto& msg : history) {
                full_prompt += msg.role + ": " + msg.content + "\n";
            }
            
            TextPrompt prompt{full_prompt};
            auto result = llm_client.chat(prompt); // Gọi phương thức chat của LLMClient
            
            if (result.has_value()) {
                response.success = true;
                response.content = result.value();
            } else {
                response.success = false;
                response.error_message = result.error().message;
            }
            return response;
        };

        // 3. Tạo callback tạo System Prompt (Gói Tầng 3)
        agent::SystemPromptFn prompt_fn = [&skill_loader](const string& extra) {
            string base_prompt = "You are a helpful AI Agent.\n";
            base_prompt += extra + "\n";
            
            // Giả sử nạp nội dung của skill_planner vào System Prompt
            Skill* planner = skill_loader.getSkill("task_planner");
            if (planner != nullptr) {
                base_prompt += "\n" + planner->getContent();
            }
            return base_prompt;
        };

        // 4. Khởi tạo cấu hình Agent và kích hoạt Loop Detector
        agent::AgentConfig config;
        config.max_steps = 10;
        config.system_prompt_extra = "Always use TOOL_CALL: tool_name(args) format.";
        
        // LoopDetector được khởi tạo ẩn bên trong hàm tạo của AgentLoop
        agent::AgentLoop agent(llm_fn, tool_fn, prompt_fn, config);
        
        cout << "   -> [OK] Da ghep noi thanh cong 5 tang!\n";
        cout << "-------------------------------------------\n";

        /* 
        // CHẠY THỬ NGHIỆM (Chỉ bỏ comment nếu Ollama đang bật)
        // cout << "\n>>> KICH HOAT AGENT CHAY THUC TE <<<\n";
        // agent::Trajectory traj = agent.run("task_test_01", "Calculate 15 * 17");
        // cout << "Ket qua cuoi cung: " << traj.final_answer << "\n";
        */

        cout << "\n[THANH CONG] BO KHUNG HE THONG DA HOAN THIEN!\n";

    } catch (const exception& e) {
        cerr << "\n[LOI NGHIEM TRONG] " << e.what() << "\n";
        return 1;
    }

    return 0;
}