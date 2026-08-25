#include "LLM_Client.h"
#include <iostream>
#include <string>
#include <print>

int main() {
    std::println("=== KHỞI TẠO LLM CLIENT (OLLAMA) ===");
    
    // Khởi tạo Ollama Client
    // Lưu ý: Đảm bảo Ollama đang chạy ở localhost:11434 với model qwen2.5
    OllamaClient llm("http://localhost:11434", "qwen2.5", 0.7f, 2048, 60000); // Timeout 60s
    
    // Thiết lập hook để in log nội bộ
    // (Gợi ý: Bạn có thể comment đoạn này lại nếu không muốn các dòng [LLM Log] in ra làm rối màn hình chat)
    llm.setLogHook([](const std::string& msg) {
        std::println("[LLM Log] {}", msg);
    });

    std::println("\n-> Hệ thống đã sẵn sàng. Gõ 'exit' hoặc 'quit' để thoát.\n");

    // Vòng lặp chat liên tục
    while (true) {
        std::string user_input;
        
        // In dấu nhắc nhập liệu
        std::print("Bạn: ");
        
        // Đọc dữ liệu người dùng nhập từ bàn phím (bao gồm cả khoảng trắng)
        if (!std::getline(std::cin, user_input)) {
            break; // Thoát nếu luồng nhập bị đóng (ví dụ: nhấn Ctrl+D)
        }

        // Kiểm tra lệnh thoát
        if (user_input == "exit" || user_input == "quit") {
            std::println("Đang thoát chương trình. Tạm biệt!");
            break;
        }

        // Xử lý trường hợp người dùng lỡ nhấn Enter mà chưa nhập gì
        if (user_input.empty()) {
            std::println("-> Vui lòng nhập nội dung (Prompt không được để trống).\n");
            continue;
        }

        // Đóng gói input và gửi request
        TextPrompt text_prompt{user_input};
        auto result = llm.chat(text_prompt);
        
        // In kết quả
        if (result.has_value()) {
            std::println("\n[Qwen2.5]:\n{}\n", result.value());
        } else {
            std::println("\n[LỖI]: {}", result.error().message);
            // Gợi ý khi mất kết nối
            if (result.error().kind == LLMErrorKind::ConnectionRefused) {
                std::println("-> Gợi ý: Hãy kiểm tra xem server Ollama đã được bật chưa (chạy lệnh 'ollama run qwen2.5').");
            }
            std::println();
        }
        
        std::println("--------------------------------------------------");
    }

    return 0;
}