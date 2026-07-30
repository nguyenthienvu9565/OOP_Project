#include "main.h"
#include "LLM_Client.h"
#include <iostream>
#include <print>
#include <memory>

// ==========================================
// CÀI ĐẶT CÁC CÔNG CỤ CỤ THỂ
// ==========================================
std::string PDFParserTool::getName() const { 
    return "PDFParser"; 
}

std::optional<std::string> PDFParserTool::execute(const std::string& input) {
    return "Đã trích xuất nội dung văn bản từ tài liệu PDF: " + input;
}

std::string WebSearchTool::getName() const { 
    return "WebSearch"; 
}

std::optional<std::string> WebSearchTool::execute(const std::string& input) {
    return "Kết quả tìm kiếm web cho: " + input;
}

// ==========================================
// CHƯƠNG TRÌNH CHÍNH
// ==========================================
int main() {
    std::println("=== KHỞI TẠO HỆ THỐNG LLM AGENT ===");
    
    // 1. Khởi tạo LLM Client (Sử dụng model qwen2.5)
    OllamaClient llm("http://localhost:11434", "qwen2.5");
    llm.setLogHook([](const std::string& msg) {
        std::println("[LLM Log] {}", msg);
    });

    // 2. Khởi tạo Tool Registry & Đăng ký công cụ (Factory Pattern)
    auto registry = std::make_shared<ToolRegistry>();
    registry->registerTool("PDFParser", []() { return std::make_unique<PDFParserTool>(); });
    registry->registerTool("WebSearch", []() { return std::make_unique<WebSearchTool>(); });

    // 3. Thiết lập Agent Loop (Template Method & Observer Pattern)
    AgentLoop agent(registry);
    agent.setHook([](const std::string& msg) {
        std::println("[Agent Step] {}", msg);
    });

    // 4. Khởi chạy tác vụ
    std::string task = "Đọc file tài liệu tiếng Việt và tự động sinh bộ câu hỏi trắc nghiệm (MCQ).";
    
    // Test gọi trực tiếp LLM Client
    std::println("\n--- KIỂM TRA LLM CLIENT ---");
    TextPrompt prompt{task};
    auto llm_result = llm.chat(prompt);
    if (llm_result.has_value()) {
        std::println("LLM Phản hồi: {}", llm_result.value());
    } else {
        std::println("LLM Báo lỗi: {}", llm_result.error());
    }

    // Test chạy quy trình Agent Loop (ReAct)
    std::println("\n--- KIỂM TRA AGENT LOOP ---");
    std::string final_result = agent.run(task);
    std::println("\n>>> KẾT QUẢ CUỐI CÙNG: {}", final_result);

    return 0;
}
