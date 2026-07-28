#include "LLM_Client.h"
#include <iostream>
#include <format> // C++20
#include <print>  // C++23

// ==========================================
// CÀI ĐẶT LLMClient
// ==========================================
void LLMClient::setLogHook(std::function<void(const std::string&)> hook) {
    log_hook = std::move(hook);
}

// ==========================================
// CÀI ĐẶT OllamaClient
// ==========================================

// Cài đặt Constructor (không ghi lại giá trị mặc định ở đây)
OllamaClient::OllamaClient(std::string url, std::string model, 
                           std::optional<float> temp, std::optional<int> tokens)
    : base_url(std::move(url)), model_name(std::move(model)), 
      temperature(temp), max_tokens(tokens) {}

// Cài đặt phương thức chat
std::expected<std::string, std::string> OllamaClient::chat(const PromptType& prompt) {
    std::string final_payload;

    // C++17: std::visit và if constexpr phân rã loại Prompt
    std::visit([this, &final_payload](auto&& arg) {
        using Type = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<Type, TextPrompt>) {
            if (log_hook) log_hook("Đang biên dịch Text Payload...");
            // C++20: std::format thay thế phép cộng chuỗi
            final_payload = std::format(R"({{"model": "{}", "prompt": "{}"}})", model_name, arg.text);
        } 
        else if constexpr (std::is_same_v<Type, MultimodalPrompt>) {
            // C++17: std::filesystem kiểm tra file
            if (!fs::exists(arg.image_path)) {
                if (log_hook) log_hook(std::format("LỖI: Không tìm thấy file {}", arg.image_path.string()));
                return; 
            }
            if (log_hook) log_hook("Đang biên dịch Multimodal Payload...");
            final_payload = std::format(R"({{"model": "{}", "prompt": "{}", "image_size": "{}"}})", 
                                        model_name, arg.text, fs::file_size(arg.image_path));
        }
    }, prompt);

    if (final_payload.empty()) {
        return std::unexpected("Payload rỗng hoặc khởi tạo thất bại.");
    }

    if (log_hook) log_hook(std::format("Đang gửi POST request tới {}/api/generate", base_url));

    // --- Nơi đây sẽ tích hợp thư viện libcurl hoặc cpr thực tế ---
    // Mô phỏng kết quả trả về từ API nội bộ
    bool api_success = true; 
    
    if (!api_success) {
        return std::unexpected("Connection Refused: Không thể kết nối tới máy chủ Ollama.");
    }

    return "Đây là câu trả lời được sinh ra từ mô hình RAG nội bộ.";
}