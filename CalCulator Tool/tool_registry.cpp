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