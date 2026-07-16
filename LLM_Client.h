#pragma once

#include <string>
#include <variant>
#include <optional>
#include <filesystem>
#include <functional>
#include <expected> // C++23

namespace fs = std::filesystem;

// ==========================================
// ĐỊNH NGHĨA KIỂU DỮ LIỆU ĐẦU VÀO (C++17)
// ==========================================
struct TextPrompt { 
    std::string text; 
};

struct MultimodalPrompt { 
    std::string text; 
    fs::path image_path; 
};

// std::variant đại diện cho 1 trong 2 loại Prompt
using PromptType = std::variant<TextPrompt, MultimodalPrompt>;

// ==========================================
// INTERFACE TRỪU TƯỢNG: LLMClient
// ==========================================
class LLMClient {
protected:
    // std::function + Lambda callback để ghi log
    std::function<void(const std::string&)> log_hook;

public:
    virtual ~LLMClient() = default;

    // Chỉ khai báo phương thức
    void setLogHook(std::function<void(const std::string&)> hook);

    // std::expected (C++23) trả về Kết quả HOẶC Lỗi
    virtual std::expected<std::string, std::string> chat(const PromptType& prompt) = 0;
};

// ==========================================
// KHAI BÁO CỤ THỂ: OllamaClient
// ==========================================
class OllamaClient : public LLMClient {
private:
    std::string base_url;
    std::string model_name;
    std::optional<float> temperature;
    std::optional<int> max_tokens;

public:
    // Khai báo Constructor với các giá trị mặc định
    OllamaClient(std::string url, std::string model = "qwen2.5", 
                 std::optional<float> temp = 0.7f, std::optional<int> tokens = 2048);

    // Khai báo phương thức override
    std::expected<std::string, std::string> chat(const PromptType& prompt) override;
};