#include "Agent_Core_New_Update.h"
#include <iostream>
#include <format>
#include <print>
#include <chrono>
#include <sstream>

// ==========================================
// TOKEN ESTIMATION (heuristic mock — no real tokenizer available,
// since think() only simulates an LLM call instead of really calling one)
// ==========================================
int estimateTokens(const std::string& text) {
    // Naive whitespace word count as a stand-in for a real tokenizer.
    // Real subword tokenizers (BPE, etc.) usually produce somewhat more
    // tokens than words, so we apply a small fixed multiplier as a rough
    // approximation. This is explicitly a MOCK metric.
    std::istringstream iss(text);
    int words = 0;
    std::string w;
    while (iss >> w) ++words;
    return static_cast<int>(words * 1.3) + (words > 0 ? 1 : 0);
}

// CÀI ĐẶT TOOL REGISTRY
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


// CÀI ĐẶT AGENT LOOP
AgentLoop::AgentLoop(std::shared_ptr<ToolRegistry> registry) 
    : tool_registry(std::move(registry)) {}

void AgentLoop::setHook(std::function<void(const std::string&)> hook) {
    step_hook = std::move(hook);
}

void AgentLoop::setTrajectoryHook(std::function<void(const TrajectoryStep&)> hook) {
    trajectory_hook = std::move(hook);
}

std::string AgentLoop::think(const std::string& task) {
    auto t0 = std::chrono::steady_clock::now();

    // Giả lập gọi LLM (Trong thực tế sẽ gọi LLMClient ở đây)
    std::string thought = std::format("LLM phân tích task: {}", task);

    auto t1 = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (step_hook) step_hook("THOUGHT: " + thought);
    if (trajectory_hook) {
        trajectory_hook(TrajectoryStep{"THOUGHT", thought, latency_ms, estimateTokens(thought)});
    }

    // Giả lập LLM quyết định dùng WebSearch
    return "WebSearch"; 
}

std::optional<std::string> AgentLoop::act(const std::string& tool_name, const std::string& input) {
    auto t0 = std::chrono::steady_clock::now();

    // FACTORY PATTERN hoạt động: Tạo tool từ tên do LLM quyết định
    auto tool = tool_registry->createTool(tool_name);

    if (!tool) {
        auto t1 = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::string content = "Không có công cụ " + tool_name;

        if (step_hook) step_hook("LỖI ACTION: " + content);
        if (trajectory_hook) {
            trajectory_hook(TrajectoryStep{"LỖI ACTION", content, latency_ms, estimateTokens(content)});
        }
        return std::nullopt;
    }

    std::string action_content = std::format("Gọi công cụ {} với input '{}'", tool_name, input);
    if (step_hook) step_hook("ACTION: " + action_content);

    auto result = tool->execute(input);

    auto t1 = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    if (trajectory_hook) {
        trajectory_hook(TrajectoryStep{"ACTION", action_content, latency_ms, estimateTokens(action_content)});
    }

    return result;
}

void AgentLoop::observe(const std::string& observation) {
    auto t0 = std::chrono::steady_clock::now();

    std::string content = "Nhận kết quả: " + observation;
    // Lưu vào Memory / Vector ở đây

    auto t1 = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (step_hook) step_hook("OBSERVATION: " + content);
    if (trajectory_hook) {
        trajectory_hook(TrajectoryStep{"OBSERVATION", content, latency_ms, estimateTokens(content)});
    }
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