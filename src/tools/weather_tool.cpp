#pragma once

#include "tool.h"
#include <curl/curl.h>
#include <string>
#include <sstream>

/**
 * @brief Tool ho tro LLM tra cuu thoi tiet hien tai cua mot thanh pho.
 * Ten dang ky: "get_weather"
 */
class WeatherTool : public Tool {
public:
    WeatherTool();
    ~WeatherTool() override = default;

    /**
     * @brief Thuc thi viec tra cuu thoi tiet.
     * @param arguments Ten thanh pho khong dau (e.g. "Hanoi", "HoChiMinh")
     * @return Chuoi mo ta thoi tiet hien tai.
     */
    std::string execute(const std::string& arguments) override;

private:
    // Callback cua libcurl de ghi du lieu
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

    // Ma hoa ten thanh pho de dua vao URL
    static std::string urlEncode(CURL* curl, const std::string& text);
};

// ==========================================
// TRIEN KHAI LOP WEATHERTOOL
// ==========================================

WeatherTool::WeatherTool()
    : Tool("get_weather",
           "Gets the current weather conditions for any city in the world. "
           "Input is a city name in English, e.g. 'Hanoi', 'HoChiMinh', 'Tokyo', 'London'. "
           "Returns a short description of current weather including temperature and conditions. "
           "No API key required. Example input: 'Hanoi'") {}

size_t WeatherTool::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string WeatherTool::urlEncode(CURL* curl, const std::string& text) {
    char* encoded = curl_easy_escape(curl, text.c_str(), static_cast<int>(text.size()));
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

std::string WeatherTool::execute(const std::string& arguments) {
    if (arguments.empty()) {
        return "Error: City name argument is empty.";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize CURL.";
    }

    std::string encoded_city = urlEncode(curl, arguments);
    std::string url = "https://wttr.in/" + encoded_city + "?format=3&lang=en";

    std::string response_buf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.0 OopAgent/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return "Error: Failed to fetch weather data: " + std::string(curl_easy_strerror(res));
    }

    if (response_buf.empty()) {
        return "Error: No weather data received for city: " + arguments;
    }

    while (!response_buf.empty() && (response_buf.back() == '\n' || response_buf.back() == '\r')) {
        response_buf.pop_back();
    }

    if (response_buf.find("Unknown location") != std::string::npos ||
        response_buf.find("Sorry") != std::string::npos) {
        return "Error: City not found: '" + arguments + "'. Try using English city name without special characters.";
    }

    return "Current weather - " + response_buf;
}
