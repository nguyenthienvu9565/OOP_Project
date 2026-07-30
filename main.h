#pragma once
#include "agent_core.h"
#include <string>
#include <optional>

// ==========================================
// KHAI BÁO CÁC CÔNG CỤ CỤ THỂ
// ==========================================
class PDFParserTool : public Tool {
public:
    std::string getName() const override;
    std::optional<std::string> execute(const std::string& input) override;
};

class WebSearchTool : public Tool {
public:
    std::string getName() const override;
    std::optional<std::string> execute(const std::string& input) override;
};
main.h
Đang hiển thị main.h.