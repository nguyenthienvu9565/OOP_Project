#include "LLM_Client.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <array>
#include <vector>

using json = nlohmann::json;

// ==========================================
// LLMClient
// ==========================================
void LLMClient::setLogHook(std::function<void(const std::string&)> hook) {
    log_hook = std::move(hook);
}

// ==========================================
// Local helpers
// ==========================================
namespace {

// Minimal, dependency-free base64 encoder (RFC 4648).
std::string base64Encode(const std::vector<unsigned char>& data) {
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= data.size()) {
        uint32_t chunk = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(table[(chunk >> 18) & 0x3F]);
        out.push_back(table[(chunk >> 12) & 0x3F]);
        out.push_back(table[(chunk >> 6) & 0x3F]);
        out.push_back(table[chunk & 0x3F]);
        i += 3;
    }

    const size_t remaining = data.size() - i;
    if (remaining == 1) {
        uint32_t chunk = data[i] << 16;
        out.push_back(table[(chunk >> 18) & 0x3F]);
        out.push_back(table[(chunk >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (remaining == 2) {
        uint32_t chunk = (data[i] << 16) | (data[i + 1] << 8);
        out.push_back(table[(chunk >> 18) & 0x3F]);
        out.push_back(table[(chunk >> 12) & 0x3F]);
        out.push_back(table[(chunk >> 6) & 0x3F]);
        out.push_back('=');
    }
    return out;
}

// Reads a file fully into memory and base64-encodes it.
// Returns std::nullopt on any I/O failure (missing file, permission denied, etc.)
std::optional<std::string> encodeImageFile(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) {
        return std::nullopt;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (buffer.empty()) {
        return std::nullopt; // treat a 0-byte image as invalid input
    }

    return base64Encode(buffer);
}

// Strips a single trailing slash so "http://host/" and "http://host" behave identically.
std::string normalizeBaseUrl(std::string url) {
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

} // namespace

// ==========================================
// OllamaClient
// ==========================================
OllamaClient::OllamaClient(std::string url, std::string model,
                            std::optional<float> temp, std::optional<int> tokens,
                            int timeout_milliseconds)
    : base_url(normalizeBaseUrl(std::move(url))),
      model_name(std::move(model)),
      temperature(temp),
      max_tokens(tokens),
      timeout_ms(timeout_milliseconds) {}

std::optional<std::string> OllamaClient::buildPayload(const PromptType& prompt) const {
    if (model_name.empty()) {
        if (log_hook) log_hook("Error: model name is empty.");
        return std::nullopt;
    }

    // Ollama's /api/chat expects a "messages" array
    json body;
    body["model"] = model_name;
    body["stream"] = false;

    json options = json::object();
    if (temperature.has_value()) options["temperature"] = *temperature;
    if (max_tokens.has_value()) options["num_predict"] = *max_tokens; 
    if (!options.empty()) body["options"] = options;

    json message = json::object();
    message["role"] = "user";

    bool build_error = false;

    std::visit([&](auto&& arg) {
        using Type = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<Type, TextPrompt>) {
            if (arg.text.empty()) {
                build_error = true;
                if (log_hook) log_hook("Error: text prompt is empty.");
                return;
            }
            message["content"] = arg.text;
            if (log_hook) log_hook("Building text payload...");
        }
        else if constexpr (std::is_same_v<Type, MultimodalPrompt>) {
            auto encoded = encodeImageFile(arg.image_path);
            if (!encoded) {
                build_error = true;
                if (log_hook) log_hook("Error: image file not found, unreadable, or empty: " + arg.image_path.string());
                return;
            }
            message["content"] = arg.text; 
            message["images"] = json::array({*encoded});
            if (log_hook) log_hook("Building multimodal payload (image base64-encoded)...");
        }
    }, prompt);

    if (build_error) {
        return std::nullopt;
    }

    body["messages"] = json::array({message});
    return body.dump();
}

std::optional<std::string> OllamaClient::chat(const PromptType& prompt) {
    auto payload = buildPayload(prompt);
    if (!payload.has_value()) {
        return std::nullopt;
    }

    const std::string url = base_url + "/api/chat";
    if (log_hook) log_hook("Sending POST request to " + url);

    cpr::Response response = cpr::Post(
        cpr::Url{url},
        cpr::Body{*payload},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Timeout{timeout_ms}
    );

    // --- Transport-level errors ---
    if (response.error) {
        if (log_hook) log_hook("Transport error [" + std::to_string(static_cast<int>(response.error.code)) + "]: " + response.error.message);
        return std::nullopt;
    }

    // --- HTTP-level errors ---
    if (response.status_code < 200 || response.status_code >= 300) {
        if (log_hook) log_hook("Server returned HTTP " + std::to_string(response.status_code) + ": " + response.text);
        return std::nullopt;
    }

    // --- JSON parsing / shape validation ---
    json parsed;
    try {
        parsed = json::parse(response.text);
    } catch (const json::parse_error& e) {
        if (log_hook) log_hook(std::string("Failed to parse JSON response: ") + e.what());
        return std::nullopt;
    }

    if (!parsed.contains("message") || !parsed["message"].contains("content") ||
        !parsed["message"]["content"].is_string()) {
        if (log_hook) log_hook("JSON response missing expected 'message.content' field.");
        return std::nullopt;
    }

    if (log_hook) log_hook("Response received successfully.");
    return parsed["message"]["content"].get<std::string>();
}