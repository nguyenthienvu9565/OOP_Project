#pragma once

#include "tool.h"
#include <string>
#include <string_view>
#include <vector>
#include <chrono>

// Bao gồm các thư viện cần thiết cho các biến thành viên (Pointers)
#include <sqlite3.h>
#include <curl/curl.h>

// ==========================================
// 1. DATABASE TOOLS (memory_tool.cpp)
// ==========================================
class SQLiteManager {
protected:
    std::string dbPath;
    sqlite3* db = nullptr;

    bool openDatabase();
    void closeDatabase();
    bool createTable();

public:
    explicit SQLiteManager(std::string_view databasePath);
    virtual ~SQLiteManager();
};

class MemorySaveTool : public Tool, public SQLiteManager {
public:
    explicit MemorySaveTool(std::string_view databasePath = "agent_memory.db");
    ~MemorySaveTool() override = default;

    std::string execute(const std::string& arguments) override;
};

class MemorySearchTool : public Tool, public SQLiteManager {
public:
    explicit MemorySearchTool(std::string_view databasePath = "agent_memory.db");
    ~MemorySearchTool() override = default;

    std::string execute(const std::string& arguments) override;
};

// ==========================================
// 2. MATH TOOL (calculator_tool.cpp)
// ==========================================
class CalculatorTool : public Tool {
private:
    int getPrecedence(char op);
    bool isOperator(char c);
    double applyOp(double a, double b, char op);
    double evaluateExpression(const std::string& tokens);

public:
    CalculatorTool();
    ~CalculatorTool() override = default;

    std::string execute(const std::string& arguments) override;
};

// ==========================================
// 3. SYSTEM TOOLS (exec_tool.cpp & datetime_tool.cpp)
// ==========================================
class ExecTool : public Tool {
public:
    ExecTool();
    ~ExecTool() override = default;

    std::string execute(const std::string& arguments) override;
};

class DateTimeTool : public Tool {
public:
    DateTimeTool();
    ~DateTimeTool() override = default;

    std::string execute(const std::string& arguments) override;

private:
    static std::tm getLocalTime();
};

// ==========================================
// 4. FILE & TEXT TOOLS (file_tool.cpp & regex_tool.cpp)
// ==========================================
class ReadFileTool : public Tool {
public:
    ReadFileTool();
    std::string execute(const std::string& arguments) override;
};

class WriteFileTool : public Tool {
public:
    WriteFileTool();
    std::string execute(const std::string& arguments) override;
};

class RegexSearchTool : public Tool {
public:
    RegexSearchTool();
    ~RegexSearchTool() override = default;

    std::string execute(const std::string& arguments) override;

private:
    static bool parseArguments(const std::string& input,
                                std::string& pattern,
                                std::string& text);
    static std::vector<std::string> findAllMatches(const std::string& pattern,
                                                    const std::string& text);
};

// ==========================================
// 5. NETWORK TOOLS (web_tool.cpp, fetch_url_tool.cpp, weather_tool.cpp)
// ==========================================
class WebSearchTool : public Tool {
public:
    WebSearchTool();
    std::string execute(const std::string& arguments) override;

private:
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    std::string build_url(CURL* curl, const std::string& query) const;
    std::string parse_response(const std::string& raw_json) const;
};

class FetchUrlTool : public Tool {
public:
    FetchUrlTool();
    ~FetchUrlTool() override = default;

    std::string execute(const std::string& arguments) override;

private:
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static std::string stripHtmlTags(const std::string& html);
    static std::string truncate(const std::string& text, size_t maxLen);
};

class WeatherTool : public Tool {
public:
    WeatherTool();
    ~WeatherTool() override = default;

    std::string execute(const std::string& arguments) override;

private:
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static std::string urlEncode(CURL* curl, const std::string& text);
};