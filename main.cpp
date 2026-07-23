#include <iostream>
#include <string>
#include <vector>
#include "agent/agent_loop.h"
using namespace agent;

// 1. Giả lập hàm xử lý LLM (LLMChatFn)
// Hàm này sẽ đóng vai trò như bộ não AI, trả lời theo từng kịch bản tùy thuộc vào số lượt chat
LLMResponse mock_llm_chat(const std::vector<Message>& history)
{
    LLMResponse res;
    res.success = true;

    // Đếm số lượt nhắn tin của user để đưa ra phản hồi phù hợp từng bước
    size_t user_turns = 0;
    for (const auto& msg : history) {
        if (msg.role == "user") user_turns++;
    }

    if (user_turns == 1) {
        // Lượt đầu tiên: LLM quyết định gọi công cụ máy tính để tính 15 * 17
        res.content = "TOOL_CALL: calculator(15*17)";
    } 
    else if (user_turns == 2) {
        // Lượt thứ hai: Sau khi nhận được kết quả từ tool, LLM đưa ra câu trả lời cuối cùng
        res.content = "FINAL_ANSWER: Ket qua cua phep tinh 15 * 17 la 255.";
    } 
    else {
        res.content = "FINAL_ANSWER: Toi da hoan thanh nhiem vu.";
    }

    return res;
}

// 2. Giả lập hàm thực thi công cụ (ToolExecuteFn)
std::string mock_tool_execute(const std::string& tool_name, const std::string& tool_args)
{
    std::cout << "   [System Tool] Kich hoat cong cu: " << tool_name << " voi tham so: " << tool_args << std::endl;
    if (tool_name == "calculator" && tool_args == "15*17") {
        return "255";
    }
    return "Unknown tool or args";
}

// 3. Giả lập hàm sinh Prompt hệ thống (SystemPromptFn)
std::string mock_system_prompt(const std::string& extra)
{
    return "Ban la mot AI Agent thong minh. " + extra;
}

// 4. Hàm Hook quan sát (StepHook) để in hành trình Agent ra màn hình theo thời gian thực
void print_step_info(const Step& step)
{
    std::cout << "\n========== [HOOK OBSERVER] STEP " << step.step_id << " ==========" << std::endl;
    std::cout << " - Loai hanh dong: " << step.action_type << std::endl;
    std::cout << " - Suy nghi cua LLM: " << step.thought << std::endl;
    if (!step.tool_name.empty()) {
        std::cout << " - Cong cu duoc goi: " << step.tool_name << "(" << step.tool_args << ")" << std::endl;
        std::cout << " - Ket qua nhan tu Tool: " << step.tool_results << std::endl;
    }
    std::cout << " - Thoi gian xu ly (Latency): " << step.latency_ms << " ms" << std::endl;
    std::cout << "============================================" << std::endl;
}

int main()
{
    std::cout << "=== KHOI DONG HE THONG AI AGENT REASONING LOOP ===" << std::endl;

    // Khoởi tạo cấu hình cho Agent và Bộ dò vòng lặp
    AgentConfig cfg;
    cfg.max_steps = 5;
    cfg.system_prompt_extra = "Hay luon su dung dung format quy dinh.";
    cfg.loop_cfg.critical_threshold = 3;
    cfg.loop_cfg.pingpong_length = 4;

    // Khởi tạo đối tượng AgentLoop và truyền các hàm giả lập vào
    AgentLoop agent(mock_llm_chat, mock_tool_execute, mock_system_prompt, cfg);

    // Dang ký hàm quan sát StepHook (Observer Pattern)
    agent.set_step_hook(print_step_info);

    // Chạy Agent với một câu hỏi giả định
    std::string task_id = "TASK-001";
    std::string user_query = "Hay tinh giup toi 15 nhan voi 17";
    
    std::cout << "\n[User] Gui yeu cau: \"" << user_query << "\"" << std::endl;
    std::cout << "-> Agent dang bat dau lam viec..." << std::endl;

    Trajectory result = agent.run(task_id, user_query);

    // In kết quả hành trình cuối cùng thu được
    std::cout << "\n================ KET QUA CUOI CUNG ================" << std::endl;
    std::cout << " - Ma nhiem vu (Task ID): " << result.task_id << std::endl;
    std::cout << " - Trang thai thanh cong: " << (result.success ? "THANH CONG" : "THAT BAI") << std::endl;
    std::cout << " - Cau tra loi cuoi cung (Final Answer): " << result.final_answer << std::endl;
    std::cout << " - Tong so buoc da di qua: " << result.steps.size() << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}