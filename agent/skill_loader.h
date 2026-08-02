#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

// -------------------------------------------------------------------------
// Lớp Skill: Đóng gói dữ liệu và hành vi của một Kỹ năng
// (Tương tự như lớp Tool)
// -------------------------------------------------------------------------
class Skill {
private:
    std::string name;
    std::vector<std::string> keywords;
    std::string content;

public:
    Skill(std::string name, std::vector<std::string> keywords, std::string content)
        : name(std::move(name)), keywords(std::move(keywords)), content(std::move(content)) {}

    virtual ~Skill() = default;

    [[nodiscard]] std::string getName() const { return name; }
    [[nodiscard]] std::string getContent() const { return content; }
    [[nodiscard]] std::vector<std::string> getKeywords() const { return keywords; }

    // Đẩy logic kiểm tra matching vào bên trong đối tượng Skill
    virtual bool isMatch(const std::string& task_description) const;
};

// -------------------------------------------------------------------------
// Lớp SkillLoader: Đóng vai trò là Registry quản lý các Skill
// (Tương tự như ToolRegistry)
// -------------------------------------------------------------------------
class SkillLoader {
private:
    // Quản lý lifetime bằng std::unique_ptr và tra cứu nhanh bằng unordered_map
    std::unordered_map<std::string, std::unique_ptr<Skill>> registry;

    // Hàm phụ trợ ẩn (private helper)
    std::vector<std::string> extractKeywords(const std::string& filename) const;

public:
    SkillLoader() = default;
    ~SkillLoader() = default;

    // Không cho phép sao chép Registry để tránh lỗi double-free bộ nhớ
    SkillLoader(const SkillLoader&) = delete;
    SkillLoader& operator=(const SkillLoader&) = delete;

    // Đăng ký một Skill vào hệ thống (Chuyển quyền sở hữu)
    bool registerSkill(std::unique_ptr<Skill> skill);

    // Quét thư mục và tự động nạp các file .md, sau đó gọi registerSkill
    bool loadFromDirectory(const std::string& directory_path);

    // Lấy con trỏ đến Skill cụ thể theo tên (không làm mất quyền sở hữu)
    Skill* getSkill(const std::string& name) const;

    // Lọc ra các Skill phù hợp với mô tả công việc (Task)
    std::vector<Skill*> selectSkills(const std::string& task_description) const;

    // Trả về chuỗi gộp nội dung của các skill phù hợp để nhúng thẳng vào Prompt
    std::string getInjectedPrompt(const std::string& task_description) const;
};