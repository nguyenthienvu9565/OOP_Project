// llm_export.hpp
// =========================================================================
// ToolRegistry::getToolsJSONSchema() trong bản của nhóm CỐ Ý xuất ra một
// khối prompt dạng plain-text ("- Tool Name: ...\n  Description: ...\n"),
// không phải JSON schema thật để truyền vào tham số `tools=[...]` của API
// LLM. Header này bổ sung lại phần đó — CHỈ thông qua interface công khai
// đã có sẵn của tool_registry.h (getAllToolNames() + getTool()), nên không
// cần và không có bất kỳ chỉnh sửa nào tới hai file tool_registry.* của
// thành viên nhóm.
// =========================================================================
#pragma once

#include "json_utils.hpp"
#include "tool_registry.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// Trả về JSON array đúng định dạng "tools=[...]" (Anthropic tool_use /
// OpenAI function-calling), chỉ gồm các tool hiện đang được phép chạy
// theo allow-list/deny-list (getAllToolNames() trả về tất cả tool đã đăng
// ký; những tool bị policy chặn vẫn hiện tên ở đây vì bản thân tool đó vẫn
// "tồn tại" — nếu muốn lọc theo policy, dùng getToolsJSONSchema() của
// ToolRegistry để lấy danh sách text đã lọc rồi đối chiếu tên).
inline std::string buildLlmToolSchemasJson(const ToolRegistry& registry) {
    std::vector<std::string> names = registry.getAllToolNames();
    std::sort(names.begin(), names.end());

    std::ostringstream oss;
    oss << "[\n";
    bool first = true;
    for (const auto& name : names) {
        const Tool* tool = registry.getTool(name);
        if (tool == nullptr) continue;  // phòng vệ; không nên xảy ra
        if (!first) oss << ",\n";
        first = false;
        oss << "  {\n"
            << "    \"name\": \"" << jsonEscape(tool->getName()) << "\",\n"
            << "    \"description\": \"" << jsonEscape(tool->getDescription()) << "\",\n"
            << "    \"input_schema\": " << tool->getParametersSchema() << "\n"
            << "  }";
    }
    oss << "\n]";
    return oss.str();
}
