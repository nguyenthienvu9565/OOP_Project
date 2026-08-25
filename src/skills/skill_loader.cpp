#include "skill_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

// =========================================================================
// Triển khai Lớp Skill
// =========================================================================

bool Skill::isMatch(const std::string& task_description) const {
    std::string task_lower = task_description;
    std::transform(task_lower.begin(), task_lower.end(), task_lower.begin(), ::tolower);

    for (const auto& kw : keywords) {
        std::string kw_lower = kw;
        std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);

        if (task_lower.find(kw_lower) != std::string::npos) {
            return true; // Khớp chỉ cần 1 keyword là đủ
        }
    }
    return false;
}

// =========================================================================
// Triển khai Lớp SkillLoader
// =========================================================================

std::vector<std::string> SkillLoader::extractKeywords(const std::string& filename) const {
    std::vector<std::string> keywords;
    std::string base_name = filename;
    
    size_t dot_pos = base_name.find_last_of('.');
    if (dot_pos != std::string::npos) {
        base_name = base_name.substr(0, dot_pos);
    }
    
    std::replace(base_name.begin(), base_name.end(), '_', ' ');
    keywords.push_back(base_name);
    return keywords;
}

bool SkillLoader::registerSkill(std::unique_ptr<Skill> skill) {
    if (!skill) return false;

    std::string name = skill->getName();
    if (registry.find(name) != registry.end()) {
        std::cerr << "[SkillLoader] Cảnh báo: Skill '" << name << "' đã tồn tại!\n";
        return false;
    }

    registry[name] = std::move(skill);
    return true;
}

bool SkillLoader::loadFromDirectory(const std::string& directory_path) {
    if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
        std::cerr << "[SkillLoader] Lỗi: Thư mục '" << directory_path << "' không hợp lệ!\n";
        return false;
    }

    int loaded_count = 0;
    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            
            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();

            std::string file_name = entry.path().filename().string();
            std::vector<std::string> keywords = extractKeywords(file_name);

            // Tạo đối tượng Skill (cấp phát động bằng std::make_unique - C++14/17)
            auto new_skill = std::make_unique<Skill>(file_name, keywords, buffer.str());
            
            // Đăng ký vào registry
            if (registerSkill(std::move(new_skill))) {
                loaded_count++;
                std::cout << "[SkillLoader] Đã nạp: " << file_name << "\n";
            }
        }
    }
    return loaded_count > 0;
}

Skill* SkillLoader::getSkill(const std::string& name) const {
    auto it = registry.find(name);
    if (it != registry.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<Skill*> SkillLoader::selectSkills(const std::string& task_description) const {
    std::vector<Skill*> selected;

    // Duyệt qua toàn bộ std::unordered_map
    for (const auto& [name, skill_ptr] : registry) {
        // Gọi hành vi kiểm tra từ chính đối tượng Skill (Tính đa hình / Đóng gói)
        if (skill_ptr->isMatch(task_description)) {
            selected.push_back(skill_ptr.get());
        }
    }
    return selected;
}

std::string SkillLoader::getInjectedPrompt(const std::string& task_description) const {
    std::vector<Skill*> matched_skills = selectSkills(task_description);
    
    if (matched_skills.empty()) {
        return ""; // Không có skill nào phù hợp
    }

    std::ostringstream prompt;
    prompt << "=== Các quy tắc chuyên môn cần tuân thủ ===\n";
    
    for (const auto* skill : matched_skills) {
        prompt << "--- [Kỹ năng: " << skill->getName() << "] ---\n"
               << skill->getContent() << "\n\n";
    }
    
    return prompt.str();
}