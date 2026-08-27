#pragma once

#include "tool.h"
#include "tool_definition.h"
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ==========================================
// TRIỂN KHAI LỚP WEBSEARCH_TOOL
// ==========================================

WebSearchTool::WebSearchTool()
    : Tool("web_search",
           "Searches the web for current information. "
           "Input is a plain text query string, e.g. 'capital of Vietnam'. "
           "Returns a short summary of the top result.") {}

size_t WebSearchTool::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    // userdata là con trỏ tới std::string mà ta truyền vào qua CURLOPT_WRITEDATA.
    // libcurl có thể gọi callback này NHIỀU LẦN cho 1 request (dữ liệu lớn
    // đến theo từng chunk), nên ta phải append() chứ không gán đè.
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(ptr, size * nmemb);
    return size * nmemb;  // Trả về đúng số byte đã xử lý — libcurl yêu cầu bắt buộc
}

std::string WebSearchTool::build_url(CURL* curl, const std::string& query) const {
    // curl_easy_escape(): mã hóa URL (URL-encode) để query có dấu cách,
    // ký tự đặc biệt (?, &, tiếng Việt có dấu...) không làm hỏng URL.
    char* encoded = curl_easy_escape(curl, query.c_str(), static_cast<int>(query.size()));
    std::string url = "https://api.duckduckgo.com/?q=" + std::string(encoded)
                     + "&format=json&no_html=1&skip_disambig=1";
    curl_free(encoded);  // BẮT BUỘC: curl_easy_escape() cấp phát bộ nhớ,
                          // phải giải phóng bằng curl_free(), không dùng delete/free thường
    return url;
}

std::string WebSearchTool::parse_response(const std::string& raw_json) const {
    try {
        auto j = json::parse(raw_json);

        std::string answer   = j.value("Answer",   "");
        std::string abstract = j.value("Abstract", "");
        std::string heading  = j.value("Heading",  "");

        std::ostringstream result;

        // DuckDuckGo trả về nhiều trường khác nhau tùy loại câu hỏi —
        // ưu tiên Answer (câu trả lời trực tiếp) trước Abstract (đoạn tóm tắt)
        if (!answer.empty()) {
            result << "Answer: " << answer << "\n";
        }
        if (!abstract.empty()) {
            if (!heading.empty()) result << "Topic: " << heading << "\n";
            result << "Summary: " << abstract << "\n";
        }

        // Nếu không có Answer/Abstract, thử lấy vài RelatedTopics
        if (answer.empty() && abstract.empty() && j.contains("RelatedTopics")) {
            int count = 0;
            for (const auto& topic : j["RelatedTopics"]) {
                if (topic.contains("Text") && count < 3) {
                    result << "- " << topic["Text"].get<std::string>() << "\n";
                    ++count;
                }
            }
        }

        std::string final_result = result.str();
        return final_result.empty() ? "No results found for this query." : final_result;
    }
    catch (const json::parse_error& e) {
        return "Error: Failed to parse search response: " + std::string(e.what());
    }
}

std::string WebSearchTool::execute(const std::string& arguments) {
    if (arguments.empty()) {
        return "Error: Search query is empty.";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize CURL.";
    }

    std::string url          = build_url(curl, arguments);
    std::string response_buf;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "OopAgent/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);        // Timeout 15s tránh treo agent mãi
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Cho phép theo redirect HTTP 3xx

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);  // BẮT BUỘC: giải phóng CURL handle, tránh leak

    if (res != CURLE_OK) {
        return "Error: Network request failed: " + std::string(curl_easy_strerror(res));
    }

    return parse_response(response_buf);
}
