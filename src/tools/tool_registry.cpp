#include "tool_registry.h"
#include <iostream>
#include <sstream>

bool ToolRegistry::registerTool(std::unique_ptr<Tool> tool) {
    if (!tool) return false;
    
    std::string toolName = tool->getName();
    if (registry.find(toolName) != registry.end()) {
        // Tool đã tồn tại trong Registry
        return false;
    }
    
    registry[toolName] = std::move(tool);
    return true;
}

Tool* ToolRegistry::getTool(const std::string& name) const {
    auto it = registry.find(name);
    if (it != registry.end()) {
        return it->second.get(); // Trả về raw pointer để sử dụng, không giữ quyền sở hữu
    }
    return nullptr;
}

bool ToolRegistry::isToolAllowed(const std::string& name) const {
    // Nếu tool không tồn tại trong hệ thống, coi như không hợp lệ
    if (registry.find(name) == registry.end()) {
        return false;
    }
    // Kiểm tra Deny List trước
    if (useDenyList && deniedTools.find(name) != deniedTools.end()) {
        return false;
    }
    // Kiểm tra Allow List
    if (useAllowList) {
        return allowedTools.find(name) != allowedTools.end();
    }
    return true;
}

std::string ToolRegistry::executeTool(const std::string& name, const std::string& arguments) {
    if (!isToolAllowed(name)) {
        return "Error: Tool '" + name + "' is blocked by security policy or does not exist.";
    }

    Tool* tool = getTool(name);
    try {
        return tool->execute(arguments);
    } catch (const std::exception& e) {
        return "Error during execution of tool '" + name + "': " + std::string(e.what());
    } catch (...) {
        return "Unknown error occurred during execution of tool '" + name + "'.";
    }
}

void ToolRegistry::setAllowList(const std::vector<std::string>& tools) {
    allowedTools.clear();
    for (const auto& t : tools) {
        allowedTools.insert(t);
    }
    useAllowList = true;
    useDenyList = false; // Ưu tiên dùng AllowList
}

void ToolRegistry::setDenyList(const std::vector<std::string>& tools) {
    deniedTools.clear();
    for (const auto& t : tools) {
        deniedTools.insert(t);
    }
    useDenyList = true;
    useAllowList = false; // Ưu tiên dùng DenyList
}

void ToolRegistry::clearPolicies() {
    allowedTools.clear();
    deniedTools.clear();
    useAllowList = false;
    useDenyList = false;
}

std::vector<std::string> ToolRegistry::getAllToolNames() const {
    std::vector<std::string> names;
    names.reserve(registry.size());
    for (const auto& [name, _] : registry) {
        names.push_back(name);
    }
    return names;
}

std::string ToolRegistry::getToolsJSONSchema() const {
    std::ostringstream oss;
    oss << "Available Tools:\n";
    
    // Duyệt qua registry bằng Range-based for và Structured Binding của C++17
    for (const auto& [name, toolPtr] : registry) {
        if (isToolAllowed(name)) {
            oss << "- Tool Name: " << name << "\n"
                << "  Description: " << toolPtr->getDescription() << "\n\n";
        }
    }
    return oss.str();
}

// ============================================================
// FACTORY FUNCTION - createDefaultRegistry()
// ============================================================
// Include các file tool trực tiếp (mỗi .cpp tự chứa cả class definition
// lẫn implementation — đây là pattern nhất quán toàn dự án)
#include "exec_tool.cpp"
#include "file_tool.cpp"
#include "web_tool.cpp"
#include "memory_tool.cpp"
// 4 tool moi mo rong (tham khao Hermes-2-Pro / OpenBMB ToolBench)
#include "datetime_tool.cpp"
#include "fetch_url_tool.cpp"
#include "weather_tool.cpp"
#include "regex_search_tool.cpp"

/**
 * @brief Tao va tra ve mot ToolRegistry da duoc nap san toan bo tool.
 *
 * Ham factory nay tap trung toan bo viec khoi tao tool vao mot cho.
 * AgentLoop hay bat ky thanh phan nao can dung tool chi can goi:
 *   auto registry = createDefaultRegistry();
 *
 * Thiet ke nay tuan thu nguyen tac Dependency Injection (DI):
 * AgentLoop nhan registry qua constructor thay vi tu tao ben trong —
 * giup de test, de swap tool khi can (vi du: dung MockTool trong test).
 *
 * @return std::unique_ptr<ToolRegistry> Registry da co du tool, ready to use
 */
std::unique_ptr<ToolRegistry> createDefaultRegistry() {
    auto registry = std::make_unique<ToolRegistry>();

    // --- Tool goc cua du an ---
    registry->registerTool(std::make_unique<ExecTool>());
    registry->registerTool(std::make_unique<WebSearchTool>());
    registry->registerTool(std::make_unique<MemorySaveTool>());
    registry->registerTool(std::make_unique<MemorySearchTool>());

    // --- 4 Tool moi mo rong (tham khao Hermes-2-Pro / OpenBMB ToolBench) ---
    registry->registerTool(std::make_unique<DateTimeTool>());      // get_datetime
    registry->registerTool(std::make_unique<FetchUrlTool>());      // fetch_url
    registry->registerTool(std::make_unique<WeatherTool>());       // get_weather
    registry->registerTool(std::make_unique<RegexSearchTool>());   // regex_search

    return registry;
}
