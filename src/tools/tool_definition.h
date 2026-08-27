#pragma once

#include "tool.h"
#include <string>
#include <string_view>

// Bao gồm các thư viện cần thiết cho các biến thành viên (Pointers)
#include <sqlite3.h>
#include <curl/curl.h>

// ==========================================
// 1. TỪ FILE: memory_tool.cpp 
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
// 2. TỪ FILE: calculator_tool.cpp 
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
// 3. TỪ FILE: exec_tool.cpp 
// ==========================================
class ExecTool : public Tool {
public:
    ExecTool();
    ~ExecTool() override = default;

    std::string execute(const std::string& arguments) override;
};

// ==========================================
// 4. TỪ FILE: file_tool.cpp 
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

// ==========================================
// 5. TỪ FILE: web_tool.cpp 
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