// tool.h
// =========================================================================
// Interface trừu tượng `Tool` mà MỌI tool (CalculatorTool, SkillTool, ...)
// phải kế thừa để đăng ký được vào ToolRegistry của nhóm
// (tool_registry.h / tool_registry.cpp).
//
// File này KHÔNG tồn tại trước đó — nó được suy ra hoàn toàn từ cách
// tool_registry.cpp thực sự dùng con trỏ Tool*:
//     tool->getName()
//     tool->getDescription()
//     tool->execute(arguments)   // arguments: std::string, trả về std::string
// và từ khai báo `std::unordered_map<std::string, std::unique_ptr<Tool>>`
// trong tool_registry.h.
//
// Hai file tool_registry.cpp / tool_registry.h của thành viên nhóm KHÔNG bị
// chỉnh sửa; file này chỉ bổ sung phần còn thiếu (định nghĩa của `Tool`)
// để chúng biên dịch được.
// =========================================================================
#pragma once

#include <string>

class Tool {
public:
    virtual ~Tool() = default;

    // Tên định danh duy nhất — đây là "key" mà ToolRegistry dùng để lưu và
    // tra cứu tool (tool_registry.cpp: registry[toolName] = ...).
    virtual std::string getName() const = 0;

    // Mô tả ngôn ngữ tự nhiên cho LLM biết khi nào/cách nào dùng tool.
    // Được ToolRegistry::getToolsJSONSchema() in ra làm prompt (dạng text).
    virtual std::string getDescription() const = 0;

    // JSON Schema (kiểu "input_schema" của Anthropic/OpenAI tool_use) mô tả
    // tham số đầu vào của tool. ToolRegistry của nhóm KHÔNG gọi tới hàm
    // này (getToolsJSONSchema() của họ chỉ xuất text thuần), nhưng nó cần
    // thiết để dựng lại JSON schema thật cho lệnh gọi API LLM — xem
    // llm_export.hpp::buildLlmToolSchemasJson(). Có triển khai mặc định để
    // các tool đơn giản (như SkillTool) không bắt buộc phải override.
    virtual std::string getParametersSchema() const { return "{}"; }

    // Thực thi tool. `arguments` là một chuỗi JSON phẳng (flat JSON object,
    // chỉ gồm các cặp key:string), ví dụ:
    //     {"action": "evaluate", "expression": "2 + 2"}
    // Trả về một chuỗi (khuyến nghị: JSON) — đây chính là nội dung sẽ được
    // feed ngược lại cho LLM như tool_result. Lỗi "mềm" (vd. biểu thức sai
    // cú pháp) nên được trả về trong chuỗi kết quả (vd. {"success":false,
    // "error": "..."}) thay vì throw, để giữ được thông tin lỗi có cấu
    // trúc; ToolRegistry::executeTool() của nhóm chỉ bắt exception cho các
    // lỗi "cứng" ngoài dự kiến (segfault logic, bad_alloc, ...).
    virtual std::string execute(const std::string& arguments) = 0;
};
