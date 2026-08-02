#include "tool.h"
#include <string>
#include <memory>
#include <array>
#include <cstdio>

// --- PHẦN KHAI BÁO (Từ exec_tool.h) ---

// Lớp ExecTool kế thừa public từ lớp trừu tượng Tool
class ExecTool : public Tool {
public:
    // Hàm khởi tạo (Constructor): Định nghĩa tên và mô tả của công cụ
    ExecTool();

    // Hàm hủy (Destructor): Đánh dấu override từ lớp cha
    ~ExecTool() override = default;

    /**
     * @brief Thực thi một câu lệnh shell Linux ngầm và trả về kết quả.
     * @param arguments Chuỗi lệnh cần chạy (Ví dụ: "ls -la", "pwd")
     * @return std::string Kết quả đầu ra (stdout/stderr) hoặc thông báo lỗi
     */
    std::string execute(const std::string& arguments) override;
};

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