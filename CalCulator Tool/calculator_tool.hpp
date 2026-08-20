// calculator_tool.hpp
// =========================================================================
// A state-of-the-art CalculatorTool for AI Agents (C++ version).
//
// ĐỒNG BỘ HÓA (2026-08-20): CalculatorTool giờ kế thừa `Tool` (tool.h) —
// interface do ToolRegistry của thành viên nhóm (tool_registry.h/.cpp) quy
// định — thay vì `ITool` của tool_registry.hpp cũ (đã bị loại bỏ khỏi dự
// án). Khác biệt chính so với bản trước:
//   - execute() nhận/trả về std::string thô (JSON phẳng) thay vì
//     ToolArgs/ToolResult của tool_registry.hpp cũ.
//   - Không còn registerInto(ToolRegistry&): việc đăng ký giờ chỉ còn một
//     dòng ở call site — registry.registerTool(std::make_unique<CalculatorTool>())
//     — vì Tool tự khai báo tên/mô tả/schema của chính nó qua các hàm ảo.
//   - CalcResult (struct nội bộ, đổi tên từ ToolResult) chỉ là chi tiết
//     triển khai riêng của CalculatorTool để gom success/error/payload
//     trước khi tự nó serialize thành MỘT chuỗi JSON trả về từ execute().
//
// Core requirement: calculates numerical expressions.
//
// Extra features on top of the base requirement:
//   - Safe evaluation via a hand-written recursive-descent parser/evaluator
//     -> there is NO call to system(), popen(), or any embedded scripting
//        engine, so an expression string can never execute arbitrary code.
//   - Scientific functions: sqrt, sin/cos/tan (+ inverse), log, log2, log10,
//     exp, factorial, floor, ceil, gcd, min/max/sum/mean over argument lists
//   - Constants: pi, e, tau, inf
//   - Degree / Radian mode toggle for trigonometric functions
//   - Variables & memory: "x = 5" stores a variable reusable in later calls
//   - `ans` variable automatically holds the last result
//   - Calculation history log, retrievable via action="history"
//   - Reset/clear via action="clear"
//   - Structured JSON result (success/result/error) so the calling agent
//     can branch on failure without parsing free text
//
// C++ standard features used in this header:
//   C++17  Abstract class / pure virtual -> CalculatorTool implements Tool
//   C++20  Three-way comparison (<=>)    -> HistoryEntry ordering
//   C++17  std::optional<T>              -> lookupVariable()
// =========================================================================
#pragma once

#include "tool.h"

#include <compare>
#include <expected>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Raised by the internal evaluator for anything it refuses or cannot
// compute (unknown variable, division by zero, unsupported syntax, ...).
class CalculatorError : public std::runtime_error {
public:
    explicit CalculatorError(const std::string& msg) : std::runtime_error(msg) {}
};

// Local, tool-private result type (renamed from the old ToolResult which
// lived in the now-removed tool_registry.hpp). The group's ToolRegistry
// only understands plain std::string, so CalcResult never crosses that
// boundary — CalculatorTool::execute() always collapses it down to a
// single JSON string before returning.
struct CalcResult {
    bool success = true;
    std::string error;
    std::string result_json;
};

// CalculatorTool implements the group's `Tool` interface (tool.h).
class CalculatorTool : public Tool {
public:
    CalculatorTool() = default;

    static const std::string kName;          // "calculator"
    static std::string description();        // LLM-facing description text
    static std::string parametersSchema();   // JSON Schema for input_schema

    // Tool overrides (tool.h). These defer to the static helpers above so
    // the description/schema text has a single source of truth.
    std::string getName() const override { return kName; }
    std::string getDescription() const override { return description(); }
    std::string getParametersSchema() const override { return parametersSchema(); }

    // The actual tool entry point. `arguments` is a flat JSON string, e.g.
    // {"action": "evaluate", "expression": "2 + 2"}; returns a JSON string
    // ({"success": true, "result": {...}} or {"success": false, "error": "..."}).
    std::string execute(const std::string& arguments) override;

private:
    struct HistoryEntry {
        std::string expression;
        double result = 0.0;
        std::string mode;

        // Three-way comparison (C++20): ordering by result lets history be
        // sorted ("largest result first", etc.) with a single
        // std::ranges::sort(..., std::greater{}) call instead of a
        // hand-written comparator. Defaulted <=> also gives us == for free.
        auto operator<=>(const HistoryEntry&) const = default;
    };

    std::map<std::string, double> variables_;
    std::vector<HistoryEntry> history_;

    // std::optional<T> (C++17): a variable genuinely may not exist yet —
    // returning optional<double> instead of throwing/defaulting to 0 makes
    // "absent" a first-class, checkable value instead of a magic number.
    std::optional<double> lookupVariable(const std::string& name) const;

    // std::expected<T,E> (C++23): runs the recursive-descent Evaluator and
    // turns its exception-based error reporting (internal to the parser,
    // where exceptions keep the grammar code readable) into a value-based
    // result at the boundary, the same way CalcResult does one level up.
    // Callers check `.has_value()` instead of wrapping every call site in
    // try/catch.
    static std::expected<double, std::string> safeEvaluate(const std::string& expr,
                                                            const std::map<std::string, double>& scope,
                                                            bool degreeMode);

    CalcResult handleClear();
    CalcResult handleHistory();
    CalcResult handleEvaluate(const std::string& rawExpression, const std::string& mode);
};
