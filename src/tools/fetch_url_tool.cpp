#include "tool.h"
#include "tool_definition.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <algorithm>

// ==========================================
// TRIEN KHAI LOP FETCHURLTOOL
// ==========================================

FetchUrlTool::FetchUrlTool()
    : Tool("fetch_url",
           "Fetches and returns the plain text content of a specific URL. "
           "Use this after web_search to read the full content of a particular page. "
           "Input must be a full URL starting with http:// or https://. "
           "Returns up to 4000 characters of cleaned text content. "
           "Example input: 'https://en.wikipedia.org/wiki/Hanoi'") {}

size_t FetchUrlTool::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string FetchUrlTool::stripHtmlTags(const std::string& html) {
    std::string result;
    result.reserve(html.size() / 2);
    bool inside_tag = false;

    for (char c : html) {
        if (c == '<') {
            inside_tag = true;
        } else if (c == '>') {
            inside_tag = false;
            result += ' ';
        } else if (!inside_tag) {
            result += c;
        }
    }

    // Rut gon khoang trang lien tiep
    std::string cleaned;
    cleaned.reserve(result.size());
    bool prev_space = false;
    for (char c : result) {
        bool is_space = (c == ' ' || c == '\t' || c == '\r' || c == '\n');
        if (is_space) {
            if (!prev_space) cleaned += ' ';
            prev_space = true;
        } else {
            cleaned += c;
            prev_space = false;
        }
    }

    return cleaned;
}

std::string FetchUrlTool::truncate(const std::string& text, size_t maxLen) {
    if (text.size() <= maxLen) return text;
    return text.substr(0, maxLen) + "\n\n[Content truncated to " + std::to_string(maxLen) + " characters]";
}

std::string FetchUrlTool::execute(const std::string& arguments) {
    if (arguments.empty()) {
        return "Error: URL argument is empty.";
    }

    if (arguments.rfind("http://", 0) != 0 && arguments.rfind("https://", 0) != 0) {
        return "Error: Invalid URL. Must start with 'http://' or 'https://'.";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize CURL.";
    }

    std::string html_buffer;
    curl_easy_setopt(curl, CURLOPT_URL, arguments.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; OopAgent/1.0)");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return "Error: Failed to fetch URL: " + std::string(curl_easy_strerror(res));
    }

    if (html_buffer.empty()) {
        return "Error: No content received from URL.";
    }

    std::string text = stripHtmlTags(html_buffer);
    return truncate(text, 4000);
}
