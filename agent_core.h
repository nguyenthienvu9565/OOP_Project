#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>

// ==========================================
// 1. TOOL & REGISTRY (FACTORY PATTERN)
// ==========================================
class Tool {
public:
    virtual ~Tool() = default;
    virtual std::string getName() const = 0;
    // C++17: std::optional trả về rỗng nếu tool chạy lỗi
    virtual std::optional<std::string> execute(const std::string& input) = 0;
};

class ToolRegistry {
private:
    // C++17: Alias cho hàm Factory tạo ra Tool
    using ToolFactory = std::function<std::unique_ptr<Tool>()>;
    std::unordered_map<std::string, ToolFactory> registry;

public:
    void registerTool(const std::string& name, ToolFactory factory);
    std::unique_ptr<Tool> createTool(const std::string& name) const;
};

// ==========================================
// 2. AGENT LOOP (TEMPLATE METHOD PATTERN)
// ==========================================
class AgentLoop {
protected:
    std::shared_ptr<ToolRegistry> tool_registry;
    
    // OBSERVER / HOOK PATTERN
    std::function<void(const std::string&)> step_hook;

    // Các bước con trong bộ khung (có thể bị ghi đè bởi lớp con)
    virtual std::string think(const std::string& task);
    virtual std::optional<std::string> act(const std::string& tool_name, const std::string& input);
    virtual void observe(const std::string& observation);

public:
    explicit AgentLoop(std::shared_ptr<ToolRegistry> registry);
    virtual ~AgentLoop() = default;

    void setHook(std::function<void(const std::string&)> hook);

    // TEMPLATE METHOD: Cố định luồng chạy (Không cho phép override)
    std::string run(const std::string& task);
};