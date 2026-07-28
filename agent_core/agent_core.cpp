#include "agent_core.h"
#include <iostream>

// Yêu cầu C++20/C++23
#include <format>
#include <print>

// ==========================================
// CÀI ĐẶT TOOL REGISTRY
// ==========================================
void ToolRegistry::registerTool(const std::string& name, ToolFactory factory) {
    registry[name] = std::move(factory);
    std::println("[Registry] Đã đăng ký công cụ: {}", name);
}

std::unique_ptr<Tool> ToolRegistry::createTool(const std::string& name) const {
    auto it = registry.find(name);
    if (it != registry.end()) {
        return it->second(); // Kích hoạt Factory function để tạo đối tượng
    }
    std::println(stderr, "[LỖI] Không tìm thấy Tool: {}", name);
    return nullptr;
}

// ==========================================
// CÀI ĐẶT AGENT LOOP
// ==========================================
AgentLoop::AgentLoop(std::shared_ptr<ToolRegistry> registry) 
    : tool_registry(std::move(registry)) {}

void AgentLoop::setHook(std::function<void(const std::string&)> hook) {
    step_hook = std::move(hook);
}

std::string AgentLoop::think(const std::string& task) {
    // Giả lập gọi LLM (Trong thực tế sẽ gọi LLMClient ở đây)
    std::string thought = std::format("LLM phân tích task: {}", task);
    if (step_hook) step_hook("THOUGHT: " + thought);
    
    // Giả lập LLM quyết định dùng WebSearch
    return "WebSearch"; 
}

std::optional<std::string> AgentLoop::act(const std::string& tool_name, const std::string& input) {
    // FACTORY PATTERN hoạt động: Tạo tool từ tên do LLM quyết định
    auto tool = tool_registry->createTool(tool_name);
    
    if (!tool) {
        if (step_hook) step_hook("LỖI ACTION: Không có công cụ " + tool_name);
        return std::nullopt;
    }

    if (step_hook) step_hook(std::format("ACTION: Gọi công cụ {} với input '{}'", tool_name, input));
    return tool->execute(input);
}

void AgentLoop::observe(const std::string& observation) {
    if (step_hook) step_hook("OBSERVATION: Nhận kết quả: " + observation);
    // Lưu vào Memory / Vector ở đây
}

// === TEMPLATE METHOD ===
// Nơi luồng ReAct được cố định vĩnh viễn
std::string AgentLoop::run(const std::string& task) {
    std::println("\n>>> KHỞI ĐỘNG AGENT VỚI TASK: {}", task);

    // Bước 1: Suy nghĩ
    std::string selected_tool = think(task);
    
    // Bước 2: Hành động
    auto tool_result = act(selected_tool, "Thông tin tìm kiếm");

    // Bước 3: Quan sát
    if (tool_result.has_value()) {
        observe(tool_result.value());
        return "Nhiệm vụ hoàn thành dựa trên dữ liệu: " + tool_result.value();
    } else {
        observe("Hành động thất bại.");
        return "Không thể hoàn thành nhiệm vụ.";
    }
}