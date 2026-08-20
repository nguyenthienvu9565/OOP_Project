// action_types.hpp
// =========================================================================
// A tiny "scripted input" model for the calculator REPL, expressed the same
// way a UI-automation agent loop would express its actions.
//
// C++17 feature: std::variant<...>
//   `Action` is a closed set of four alternatives: Click | TypeText |
//   KeyPress | Done. For this calculator project there is no mouse, so the
//   mapping is: Click = a no-op / "focus" pulse (useful as a script
//   separator), TypeText = feed an expression into the calculator,
//   KeyPress = a named command ("history", "clear"), Done = stop the
//   script. This lets a "demo script" be written as ordinary data
//   (a std::vector<Action>) instead of ad-hoc string parsing.
//
// C++17 feature: if constexpr / std::visit
//   `runScript` dispatches every alternative through a single
//   `std::visit` call with an overloaded-lambda visitor. Each lambda
//   branch is selected entirely at compile time based on which
//   alternative is active — no runtime type id checks, no dynamic_cast.
//
// ĐỒNG BỘ HÓA (2026-08-20): dùng ToolRegistry của nhóm (tool_registry.h) —
// executeTool(name, arguments) giờ nhận/trả về std::string thô (JSON
// phẳng) thay vì ToolArgs/ToolResult cũ, và không phải hàm const (bản của
// nhóm không đánh dấu const), nên `runScript` nhận `ToolRegistry&` thay vì
// `const ToolRegistry&`.
// =========================================================================
#pragma once

#include "json_utils.hpp"
#include "tool_registry.h"

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

struct Click {};                      // a no-op / focus pulse
struct TypeText { std::string text; };  // an expression to evaluate
struct KeyPress { std::string key; };   // "history" | "clear"
struct Done {};                        // end of script

using Action = std::variant<Click, TypeText, KeyPress, Done>;

// Small helper to build an overload set out of several lambdas — the
// idiomatic pattern for turning a set of lambdas into a single callable
// that `std::visit` can dispatch through.
template <typename... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};
template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;  // class template argument deduction (C++17)

// Plays a sequence of Actions against the calculator tool registered as
// "calculator" in `registry`. Returns false as soon as a `Done` action is
// seen (or the vector is exhausted).
inline bool runScript(const std::vector<Action>& script, ToolRegistry& registry) {
    for (const Action& action : script) {  // range-based for + auto-deduced ref
        bool stop = false;
        std::visit(
            Overload{
                [&](const Click&) {
                    std::cout << "[macro] (click)\n";
                },
                [&](const TypeText& t) {
                    std::string args = buildFlatJsonObject({{"action", "evaluate"}, {"expression", t.text}});
                    std::string result = registry.executeTool("calculator", args);
                    if (jsonIndicatesFailure(result)) {
                        std::cout << "[macro] " << t.text << " -> LOI: " << result << "\n";
                    } else {
                        std::cout << "[macro] " << t.text << " -> " << result << "\n";
                    }
                },
                [&](const KeyPress& k) {
                    std::string args = buildFlatJsonObject({{"action", k.key}});
                    std::string result = registry.executeTool("calculator", args);
                    std::cout << "[macro] <" << k.key << "> -> " << result << "\n";
                },
                [&](const Done&) {
                    stop = true;
                },
            },
            action);
        if (stop) return false;
    }
    return true;
}
