// main.cpp
// =========================================================================
// Interactive Mode: Cho phép người dùng trực tiếp nhập biểu thức từ bàn phím.
// Also demonstrates: loading extra tools from ./skills at runtime, and
// replaying a scripted Action sequence (std::variant-based) as a
// self-contained smoke test.
//
// ĐỒNG BỘ HÓA (2026-08-20): dùng ToolRegistry của nhóm (tool_registry.h /
// tool_registry.cpp) làm gốc:
//   - registerToolObject(unique_ptr<ITool>) -> registerTool(unique_ptr<Tool>)
//   - executeTool(name, ToolArgs) -> ToolResult
//         -> executeTool(name, string arguments) -> string
//   - getLlmToolSchemasJson() (không còn tồn tại trong bản của nhóm)
//         -> buildLlmToolSchemasJson(registry) (llm_export.hpp, tự viết
//            thêm, chỉ dùng interface công khai getAllToolNames()/getTool())
// =========================================================================
#include "action_types.hpp"
#include "calculator_tool.hpp"
#include "json_utils.hpp"
#include "llm_export.hpp"
#include "skill_loader.hpp"
#include "tool_registry.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

int main() {
    // 1. Khởi tạo registry (bản của thành viên nhóm).
    ToolRegistry registry;

    // 2. Đăng ký CalculatorTool lúc runtime — registerTool nhận thẳng
    //    unique_ptr<Tool>; tên/mô tả/schema được đọc ra từ chính đối
    //    tượng qua getName()/getDescription()/getParametersSchema().
    registry.registerTool(std::make_unique<CalculatorTool>());

    // 2b. (Optional) load any extra tools described by ./skills/*.skill
    //     files — dynamic registration taken to its logical conclusion:
    //     the *set* of tools is decided at runtime from the filesystem.
    auto env = std::make_shared<FileSystemEnvironment>(std::filesystem::path("skills"));
    SkillLoader skillLoader(env);
    for (auto& tool : skillLoader.loadAll()) {
        registry.registerTool(std::move(tool));
    }

    // (Tuỳ chọn) In ra LLM Schema nếu bạn vẫn muốn kiểm tra. ToolRegistry
    // của nhóm chỉ xuất một prompt block dạng text qua getToolsJSONSchema();
    // JSON thật cho tham số tools=[...] của API LLM được dựng lại bởi
    // buildLlmToolSchemasJson() (llm_export.hpp).
    std::cout << "=== LLM tool schema (JSON that goes into tools=[...]) ===\n";
    std::cout << buildLlmToolSchemasJson(registry) << "\n\n";
    std::cout << "=== Prompt block (ToolRegistry::getToolsJSONSchema(), plain text) ===\n";
    std::cout << registry.getToolsJSONSchema() << "\n";

    // 3. Chế độ tương tác (Interactive loop)
    std::cout << "==========================================\n";
    std::cout << "   CHẾ ĐỘ MÁY TÍNH TƯƠNG TÁC (INTERACTIVE)  \n";
    std::cout << "==========================================\n";
    std::cout << "Hướng dẫn:\n";
    std::cout << " - Nhập biểu thức toán học (VD: 2 + 3 * 4, sin(30)).\n";
    std::cout << " - Gõ 'history' để xem các phép tính trước đó.\n";
    std::cout << " - Gõ 'clear' để xoá biến nhớ và lịch sử.\n";
    std::cout << " - Gõ 'demo' để chạy một kịch bản mẫu (scripted Action).\n";
    std::cout << " - Gõ 'exit' hoặc 'quit' để thoát chương trình.\n\n";

    std::string userInput;
    while (true) {
        std::cout << ">> ";

        // Đọc 1 dòng từ bàn phím
        if (!std::getline(std::cin, userInput)) {
            break;  // Thoát nếu gặp EOF (Ctrl+D / Ctrl+Z)
        }

        // Xử lý lệnh thoát
        if (userInput == "exit" || userInput == "quit") {
            std::cout << "Đã thoát chương trình.\n";
            break;
        }

        // Bỏ qua nếu người dùng chỉ nhấn Enter
        if (userInput.empty()) {
            continue;
        }

        if (userInput == "demo") {
            // std::variant + std::visit + if constexpr (C++17): a scripted
            // sequence of Actions, replayed through runScript().
            std::vector<Action> script{
                Click{},
                TypeText{"x = 12 / 4"},
                TypeText{"sqrt(144) + 0.15 * 200"},
                TypeText{"ans / 6"},
                KeyPress{"history"},
                Done{},
                TypeText{"this line should never run"},
            };
            runScript(script, registry);
            continue;
        }

        // 4. Đóng gói dữ liệu đầu vào thành một chuỗi JSON phẳng (thay vì
        //    ToolArgs map cũ), vì Tool::execute() của nhóm chỉ nhận string.
        std::map<std::string, std::string> fields;

        if (userInput == "history") {
            fields["action"] = "history";
        } else if (userInput == "clear") {
            fields["action"] = "clear";
        } else {
            fields["action"] = "evaluate";
            fields["expression"] = userInput;
            // Mặc định hệ thống dùng "radian". Nếu bạn muốn tính theo "degree"
            // thì có thể hardcode fields["mode"] = "degree"; ở đây.
        }
        std::string arguments = buildFlatJsonObject(fields);

        // 5. Thực thi tool qua ToolRegistry của nhóm (đã áp allow/deny-list
        //    policy nếu có cấu hình — xem setAllowList/setDenyList).
        std::string result = registry.executeTool("calculator", arguments);

        // 6. In kết quả trả về. jsonIndicatesFailure() chỉ soi chuỗi
        //    "success": false do chính CalculatorTool::execute() phát ra —
        //    xem json_utils.hpp.
        if (jsonIndicatesFailure(result)) {
            std::cout << " -> Lỗi: " << result << "\n\n";
        } else {
            std::cout << " -> Thành công: " << result << "\n\n";
        }
    }

    return 0;
}
