#pragma once

#include <string>
#include <variant>
#include <optional>
#include <filesystem>
#include <functional>
#include <cstdint>

namespace fs = std::filesystem;

// ==========================================
// INPUT DATA TYPES (C++17)
// ==========================================
struct TextPrompt {
    std::string text;
};

struct MultimodalPrompt {
    std::string text;
    fs::path image_path;
};

// std::variant represents one of the two Prompt kinds
using PromptType = std::variant<TextPrompt, MultimodalPrompt>;

// ==========================================
// ABSTRACT INTERFACE: LLMClient
// ==========================================
class LLMClient {
protected:
    // std::function + lambda callback used for logging
    std::function<void(const std::string&)> log_hook;

public:
    virtual ~LLMClient() = default;

    // Registers an optional logging callback; safe to call with nullptr to disable logging.
    void setLogHook(std::function<void(const std::string&)> hook);

    // std::optional (C++17): returns the model's reply OR std::nullopt on failure.
    virtual std::optional<std::string> chat(const PromptType& prompt) = 0;
};

// ==========================================
// CONCRETE IMPLEMENTATION: OllamaClient
// ==========================================
class OllamaClient : public LLMClient {
private:
    std::string base_url;
    std::string model_name;
    std::optional<float> temperature;
    std::optional<int> max_tokens;
    int timeout_ms; // request timeout, configurable

    // Builds the JSON payload for /api/chat given a resolved prompt.
    // Returns std::nullopt on invalid input (e.g. missing image file).
    std::optional<std::string> buildPayload(const PromptType& prompt) const;

public:
    // Constructor with sensible defaults for everything except the URL.
    explicit OllamaClient(std::string url,
                           std::string model = "qwen2.5",
                           std::optional<float> temp = 0.7f,
                           std::optional<int> tokens = 2048,
                           int timeout_milliseconds = 30000);

    std::optional<std::string> chat(const PromptType& prompt) override;

    // Simple configuration accessors (useful for tests / debugging).
    const std::string& baseUrl() const { return base_url; }
    const std::string& modelName() const { return model_name; }
    std::optional<float> temp() const { return temperature; }
    std::optional<int> maxTokens() const { return max_tokens; }
};