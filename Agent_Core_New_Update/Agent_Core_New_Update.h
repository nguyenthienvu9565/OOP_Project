
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
// 1b. STRUCTURED TRAJECTORY STEP (for Harness/Evaluator)
// ==========================================
// One instrumented step of a run: what kind of step it was (THOUGHT / ACTION /
// OBSERVATION / error variants — taken verbatim from the existing hook's prefix),
// its textual content, how long it took, and an estimated token count.
//
// NOTE on `tokens`: there is no real LLM call in this codebase (think() just
// formats a string), so there is no real tokenizer either. `tokens` is a
// heuristic estimate (whitespace word count) computed in agent_core.cpp —
// it is a MOCK metric, not an actual token count from a model/tokenizer.
struct TrajectoryStep {
    std::string type;     // "THOUGHT", "ACTION", "OBSERVATION", "LỖI ACTION", ...
    std::string content;  // text content of the step
    double latency_ms = 0.0;
    int tokens = 0;       // heuristic estimate, see note above
};
 
// ==========================================
// 2. AGENT LOOP (TEMPLATE METHOD PATTERN)
// ==========================================
class AgentLoop {
protected:
    std::shared_ptr<ToolRegistry> tool_registry;
    
    // OBSERVER / HOOK PATTERN (original, untouched — kept for backward compatibility)
    std::function<void(const std::string&)> step_hook;
 
    // OBSERVER / HOOK PATTERN (additive) — structured version used by the Harness.
    // Fired in addition to step_hook, with real latency + estimated tokens attached.
    std::function<void(const TrajectoryStep&)> trajectory_hook;
 
    // Các bước con trong bộ khung (có thể bị ghi đè bởi lớp con)
    virtual std::string think(const std::string& task);
    virtual std::optional<std::string> act(const std::string& tool_name, const std::string& input);
    virtual void observe(const std::string& observation);
 
public:
    explicit AgentLoop(std::shared_ptr<ToolRegistry> registry);
    virtual ~AgentLoop() = default;
 
    void setHook(std::function<void(const std::string&)> hook);
    void setTrajectoryHook(std::function<void(const TrajectoryStep&)> hook);
 
    // TEMPLATE METHOD: Cố định luồng chạy (Không cho phép override)
    std::string run(const std::string& task);
};
 
// Rough, tokenizer-free token estimate. Exposed so the Harness/Evaluators
// can reuse the exact same heuristic if needed.
int estimateTokens(const std::string& text);
