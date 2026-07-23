#pragma once

#include "tool.h"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp> 

using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * @brief Tool hỗ trợ LLM đọc nội dung từ một file.
 * Tên đăng ký: "read_file"
 */
class ReadFileTool : public Tool {
public:
    ReadFileTool();
    
    /**
     * @brief Thực thi việc đọc file.
     * @param arguments Đường dẫn đến file (chuỗi thuần túy hoặc JSON dạng {"path": "..."})
     * @return Nội dung file nếu thành công, hoặc chuỗi thông báo lỗi bắt đầu bằng "Error:"
     */
    std::string execute(const std::string& arguments) override;
};

/**
 * @brief Tool hỗ trợ LLM ghi nội dung vào một file.
 * Tên đăng ký: "write_file"
 */
class WriteFileTool : public Tool {
public:
    WriteFileTool();
    
    /**
     * @brief Thực thi việc ghi file.
     * @param arguments Chuỗi định dạng JSON chứa đường dẫn và nội dung cần ghi:
     * e.g., {"path": "result.txt", "content": "Dữ liệu cần ghi"}
     * @return Thông báo trạng thái thành công hoặc thông báo lỗi.
     */
    std::string execute(const std::string& arguments) override;
};

// ==========================================
// TRIỂN KHAI LỚP READ_FILE_TOOL
// ==========================================

ReadFileTool::ReadFileTool()
    : Tool("read_file", 
           "Reads the full content of a file. "
           "Input can be a raw file path string or a JSON object: {\"path\": \"file_path\"}") {}

std::string ReadFileTool::execute(const std::string& arguments) {
    std::string filePath = arguments;

    // Trích xuất đường dẫn nếu LLM truyền vào dưới dạng chuỗi JSON
    if (!arguments.empty() && arguments.front() == '{') {
        try {
            auto j = json::parse(arguments);
            if (j.contains("path") && j["path"].is_string()) {
                filePath = j["path"].get<std::string>();
            }
        } catch (const json::parse_error& e) {
            return "Error: Invalid JSON format in arguments: " + std::string(e.what());
        }
    }

    // Xử lý chuỗi rỗng
    if (filePath.empty()) {
        return "Error: File path is empty.";
    }

    // Kiểm tra sự tồn tại của file bằng std::filesystem (C++17)
    if (!fs::exists(filePath)) {
        return "Error: File '" + filePath + "' does not exist.";
    }
    if (!fs::is_regular_file(filePath)) {
        return "Error: '" + filePath + "' is not a regular file.";
    }

    // Tiến hành đọc dữ liệu từ file
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return "Error: Could not open file '" + filePath + "' for reading.";
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}


// ==========================================
// TRIỂN KHAI LỚP WRITE_FILE_TOOL
// ==========================================

WriteFileTool::WriteFileTool()
    : Tool("write_file", 
           "Writes or overwrites content to a specified file. "
           "Arguments MUST be a valid JSON object string: {\"path\": \"file_path\", \"content\": \"text_to_write\"}") {}

std::string WriteFileTool::execute(const std::string& arguments) {
    std::string filePath;
    std::string content;

    // Phân rã tham số JSON bắt buộc
    try {
        auto j = json::parse(arguments);
        
        if (!j.contains("path") || !j["path"].is_string()) {
            return "Error: Missing or invalid 'path' field in write_file arguments.";
        }
        if (!j.contains("content") || !j["content"].is_string()) {
            return "Error: Missing or invalid 'content' field in write_file arguments.";
        }

        filePath = j["path"].get<std::string>();
        content = j["content"].get<std::string>();
    } 
    catch (const json::parse_error& e) {
        // Fallback linh hoạt: Nếu LLM vô tình truyền dạng "path,content" thay vì JSON
        return "Error: write_file requires a JSON argument. Exception: " + std::string(e.what());
    }

    if (filePath.empty()) {
        return "Error: File path is empty.";
    }

    // Tự động tạo thư mục cha nếu chưa tồn tại nhờ std::filesystem (C++17)
    fs::path p(filePath);
    if (p.has_parent_path() && !fs::exists(p.parent_path())) {
        try {
            fs::create_directories(p.parent_path());
        } catch (const fs::filesystem_error& e) {
            return "Error: Could not create directories for path '" + filePath + "'. Reason: " + e.what();
        }
    }

    // Thực hiện ghi đè dữ liệu vào file
    std::ofstream file(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return "Error: Could not open file '" + filePath + "' for writing.";
    }

    file << content;
    file.close();

    return "Success: Successfully wrote data to '" + filePath + "'.";
}
