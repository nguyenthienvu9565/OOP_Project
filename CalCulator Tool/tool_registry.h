#pragma once

#include "tool.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <optional>

class ToolRegistry {
private:
    // Sử dụng std::unique_ptr để quản lý tự động lifetime của các Tool (C++17)
    std::unordered_map<std::string, std::unique_ptr<Tool>> registry;

    // Danh sách Tool Policy (Allowlist / Denylist)
    std::unordered_set<std::string> allowedTools;
    std::unordered_set<std::string> deniedTools;
    bool useAllowList = false;
    bool useDenyList = false;

public:
    ToolRegistry() = default;
    ~ToolRegistry() = default;

    // Không cho phép sao chép Registry để tránh xung đột quản lý tài nguyên
    ToolRegistry(const ToolRegistry&) = delete;
    ToolRegistry& operator=(const ToolRegistry&) = delete;

    // Đăng ký một Tool mới vào hệ thống (Chuyển quyền sở hữu qua std::unique_ptr)
    bool registerTool(std::unique_ptr<Tool> tool);

    // Lấy con trỏ đến Tool dựa theo tên
    Tool* getTool(const std::string& name) const;

    // Thực thi một Tool cụ thể kèm kiểm tra chính sách Policy thông qua tên
    std::string executeTool(const std::string& name, const std::string& arguments);

    // Cấu hình Tool Policy
    void setAllowList(const std::vector<std::string>& tools);
    void setDenyList(const std::vector<std::string>& tools);
    void clearPolicies();

    // Sinh chuỗi định dạng mô tả toàn bộ Tool hợp lệ để nạp vào System Prompt cho LLM
    std::string getToolsJSONSchema() const;

    // Lấy danh sách tên của tất cả các tool đang có trong hệ thống
    std::vector<std::string> getAllToolNames() const;

private:
    // Kiểm tra xem Tool có được phép chạy dựa trên Policy hiện tại hay không
    bool isToolAllowed(const std::string& name) const;
};