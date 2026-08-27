#include "tool.h"
#include "tool_definition.h"
#include <string>
#include <memory>
#include <array>
#include <cstdio>

// --- PHẦN ĐỊNH NGHĨA (Từ exec_tool.cpp) ---

// Định nghĩa Constructor: Gọi constructor lớp cha và truyền giá trị
ExecTool::ExecTool() : Tool(
    "exec", 
    "Executes a native shell command in the Linux environment and returns its stdout and stderr. "
    "Argument must be a raw string of the command (e.g., 'ls -la')."
) {}

// Định nghĩa hàm xử lý chi tiết
std::string ExecTool::execute(const std::string& arguments) {
    if (arguments.empty()) {
        return "Error: Command argument is empty.";
    }

    std::string commandWithStderr = arguments + " 2>&1";
    std::string result;
    std::array<char, 256> buffer{};

    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(commandWithStderr.c_str(), "r"), 
        pclose
    );

    if (!pipe) {
        return "Error: Failed to launch shell command.";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    if (result.empty()) {
        return "Command executed successfully with no output.";
    }

    return result;
}