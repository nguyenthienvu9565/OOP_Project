// json_utils.hpp
// -----------------------------------------------------------------------
// Tiny hand-rolled JSON helpers. No external dependency (no nlohmann/json)
// so the whole tool kit builds with nothing but the standard library.
//
// BỔ SUNG (đồng bộ với ToolRegistry của nhóm): Tool::execute() giờ nhận
// đối số như MỘT chuỗi (thay vì std::map<string,string> ToolArgs cũ), nên
// cần thêm một parser JSON phẳng tối giản (parseFlatJsonObject) và hàm
// dựng ngược lại (buildFlatJsonObject) để bên gọi (main.cpp,
// action_types.hpp) đóng gói tham số thành chuỗi trước khi gọi
// registry.executeTool(name, arguments).
// -----------------------------------------------------------------------
#pragma once

#include <cctype>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

// -----------------------------------------------------------------------
// C++26 feature: Pack Indexing (P2662R3).
//
// `firstOf(args...)` returns whichever of several fallback strings should
// be used first (the pattern used below by CalculatorTool when picking a
// default "mode" string). On a compiler with pack indexing implemented,
// `args...[0]` reaches straight into the parameter pack by index — no
// recursion, no std::get<>, no tuple materialization. GCC 13 (used to
// build this project today) does not yet define __cpp_pack_indexing, so
// the `#else` branch — the pre-C++26 way of doing the same thing via
// std::forward_as_tuple + std::get<0> — is what actually runs; flip to a
// C++26-complete compiler and the top branch takes over automatically.
// -----------------------------------------------------------------------
template <typename... Args>
constexpr decltype(auto) firstOf(Args&&... args) {
#if defined(__cpp_pack_indexing)
    return args...[0];
#else
    return std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
#endif
}

inline std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            default:   out += c;
        }
    }
    return out;
}

// Formats a double the way you'd want it inside a JSON payload:
// integral values print without a trailing ".0" noise, others keep enough
// precision to round-trip. NaN/Infinity are not valid JSON literals, so
// they are encoded as JSON strings instead.
inline std::string formatNumber(double v) {
    if (std::isnan(v)) return "\"NaN\"";
    if (std::isinf(v)) return v > 0 ? "\"Infinity\"" : "\"-Infinity\"";

    std::ostringstream oss;
    oss.precision(12);
    oss << v;
    return oss.str();
}

// Parses a FLAT JSON object whose values are all plain strings, e.g.
//   {"action": "evaluate", "expression": "2 + 2"}
// into a std::map<std::string, std::string>. Intentionally minimal — no
// nested objects/arrays — because that is all Tool::execute() arguments
// ever need to carry in this project. Malformed/partial input simply
// yields whatever key/value pairs were parsed before the parser gave up,
// rather than throwing; a tool decides for itself how to react to
// missing/bad arguments (see CalculatorTool::execute).
inline std::map<std::string, std::string> parseFlatJsonObject(const std::string& json) {
    std::map<std::string, std::string> out;
    size_t i = 0, n = json.size();

    auto skipWs = [&]() {
        while (i < n && std::isspace(static_cast<unsigned char>(json[i]))) i++;
    };

    auto parseString = [&]() -> std::string {
        std::string s;
        if (i >= n || json[i] != '"') return s;
        i++;  // opening quote
        while (i < n && json[i] != '"') {
            if (json[i] == '\\' && i + 1 < n) {
                char c = json[i + 1];
                switch (c) {
                    case 'n': s += '\n'; break;
                    case 't': s += '\t'; break;
                    case 'r': s += '\r'; break;
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    default: s += c;
                }
                i += 2;
            } else {
                s += json[i++];
            }
        }
        if (i < n) i++;  // closing quote
        return s;
    };

    skipWs();
    if (i >= n || json[i] != '{') return out;  // not an object -> empty map
    i++;
    skipWs();
    if (i < n && json[i] == '}') return out;  // "{}"

    while (i < n) {
        skipWs();
        std::string key = parseString();
        skipWs();
        if (i < n && json[i] == ':') i++;
        skipWs();

        std::string value;
        if (i < n && json[i] == '"') {
            value = parseString();
        } else {
            // Bare literal (number/true/false/null) — take verbatim up to
            // the next ',' or '}' and trim trailing whitespace.
            size_t start = i;
            while (i < n && json[i] != ',' && json[i] != '}') i++;
            value = json.substr(start, i - start);
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
        }

        if (!key.empty()) out[key] = value;

        skipWs();
        if (i < n && json[i] == ',') { i++; continue; }
        if (i < n && json[i] == '}') { i++; break; }
        break;  // malformed — stop parsing rather than loop forever
    }
    return out;
}

// The inverse of parseFlatJsonObject: builds `{"k1": "v1", "k2": "v2"}`
// from a flat string->string map, used to pack Tool::execute() arguments
// before calling ToolRegistry::executeTool(name, arguments).
inline std::string buildFlatJsonObject(const std::map<std::string, std::string>& fields) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [k, v] : fields) {
        if (!first) oss << ", ";
        first = false;
        oss << "\"" << jsonEscape(k) << "\": \"" << jsonEscape(v) << "\"";
    }
    oss << "}";
    return oss.str();
}

// Quick, non-parsing check used by callers that only need to know whether
// a JSON string returned from Tool::execute() represents a failure, e.g.
// to pick "Thành công" vs "Lỗi" in the REPL. Relies on this project's own
// tools always emitting `"success": false` verbatim on failure (see
// CalculatorTool::execute / SkillTool::execute) — it is not a general
// JSON boolean parser.
inline bool jsonIndicatesFailure(const std::string& json) {
    return json.find("\"success\": false") != std::string::npos;
}
