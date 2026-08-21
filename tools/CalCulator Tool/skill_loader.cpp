// skill_loader.cpp
#include "skill_loader.hpp"
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

std::vector<fs::path> FileSystemEnvironment::listSkillFiles() const {
    std::vector<fs::path> out;
    std::error_code ec;
    if (!fs::exists(directory_, ec) || !fs::is_directory(directory_, ec)) {
        return out;  // no skills/ directory: perfectly normal, just nothing to load
    }
    // std::filesystem (C++17) + range-based for + auto: walk the directory
    // exactly once, no manual readdir() bookkeeping.
    for (const auto& entry : fs::directory_iterator(directory_, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".skill") {
            out.push_back(entry.path());
        }
    }
    return out;
}

std::string FileSystemEnvironment::readFile(const fs::path& path) const {
    std::ifstream in(path);
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

std::optional<SkillDescriptor> SkillLoader::parse(const std::string& contents) {
    SkillDescriptor d;
    std::istringstream iss(contents);
    std::string line;
    while (std::getline(iss, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        if (key == "name") d.name = value;
        else if (key == "description") d.description = value;
        else if (key == "reply") d.reply = value;
    }
    if (d.name.empty()) return std::nullopt;
    return d;
}

std::vector<std::unique_ptr<Tool>> SkillLoader::loadAll() {
    std::vector<std::unique_ptr<Tool>> tools;
    for (const auto& path : env_->listSkillFiles()) {
        std::optional<SkillDescriptor> parsed = parse(env_->readFile(path));
        if (!parsed.has_value()) continue;

        // shared_ptr (C++17): the descriptor is owned jointly by the
        // loader's cache (for future reload/inspection) and by the
        // SkillTool built from it below.
        auto descriptor = std::make_shared<SkillDescriptor>(std::move(parsed.value()));
        descriptors_.insert(descriptor->name, descriptor, /*overwrite=*/true);

        // unique_ptr (C++17): the caller (main.cpp) takes exclusive
        // ownership of the Tool wrapper and hands it straight to
        // ToolRegistry::registerTool.
        tools.push_back(std::make_unique<SkillTool>(descriptor));
    }
    return tools;
}
