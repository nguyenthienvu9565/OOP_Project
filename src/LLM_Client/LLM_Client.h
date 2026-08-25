#pragma once

#include <string>
#include <variant>
#include <optional>
#include <filesystem>
#include <functional>
#include <expected>
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
// ERROR TAXONOMY
// ==========================================
// Distinguishing error *kind* lets callers decide whether to retry
// (Timeout / ConnectionRefused are often retryable) or not
// (InvalidArgument / InvalidResponse usually are not).
enum class LLMErrorKind {
    InvalidArgument,   // bad input before any network call (missing file, empty prompt...)
    ConnectionRefused, // could not reach the server at all
    Timeout,           // request exceeded the configured timeout
    HttpError,         // server responded with a non-2xx status code
    InvalidResponse,   // response body was not parseable / not the expected shape
    Unknown            // anything else (kept generic on purpose)
};

struct LLMError {
    LLMErrorKind kind;
    std::string message;
};

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

    // std::expected (C++23): returns the model's reply OR a structured LLMError.
    virtual std::expected<std::string, LLMError> chat(const PromptType& prompt) = 0;
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
    // Returns unexpected on invalid input (e.g. missing image file).
    std::expected<std::string, LLMError> buildPayload(const PromptType& prompt) const;

public:
    // Constructor with sensible defaults for everything except the URL.
    explicit OllamaClient(std::string url,
                           std::string model = "qwen2.5",
                           std::optional<float> temp = 0.7f,
                           std::optional<int> tokens = 2048,
                           int timeout_milliseconds = 30000);

    std::expected<std::string, LLMError> chat(const PromptType& prompt) override;

    // Simple configuration accessors (useful for tests / debugging).
    const std::string& baseUrl() const { return base_url; }
    const std::string& modelName() const { return model_name; }
    std::optional<float> temp() const { return temperature; }
    std::optional<int> maxTokens() const { return max_tokens; }
};