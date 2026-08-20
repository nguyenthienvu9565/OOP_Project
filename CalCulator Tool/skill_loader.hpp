// skill_loader.hpp
// =========================================================================
// Loads extra "skill" tools from simple descriptor files on disk at
// runtime, so new tools can be added to a running agent without touching
// or recompiling any C++ — just drop a `.skill` file into the skills
// directory. This is the same "dynamic registration" idea as
// CalculatorTool::registerInto, taken one step further: even the *set* of
// available tools is decided at runtime, from the filesystem, not from
// source code.
//
// A `.skill` file is a tiny `key=value` text format, one pair per line:
//
//     name=echo_ping
//     description=Tra loi "pong" cho bat ky dau vao nao (health-check tool).
//     reply=pong
//
// C++ standard features used in this header:
//   C++17  std::filesystem            -> SkillLoader scans a directory
//   C++17  Abstract class/pure virtual -> IEnvironment
//   C++17  std::shared_ptr             -> SkillDescriptor is shared between
//                                         the loader's cache and every
//                                         SkillTool built from it
//   C++17  std::unique_ptr             -> buildTools() hands back owned
//                                         ITool instances
// =========================================================================
#pragma once

#include "generic_registry.hpp"
#include "json_utils.hpp"
#include "tool.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct SkillDescriptor {
    std::string name;
    std::string description;
    std::string reply;
};

// IEnvironment: an abstraction over "where do skill files come from".
// FileSystemEnvironment (below) is the only implementation shipped here,
// but tests or a future sandboxed agent could supply an in-memory
// implementation without SkillLoader knowing the difference.
class IEnvironment {
public:
    virtual ~IEnvironment() = default;
    virtual std::vector<std::filesystem::path> listSkillFiles() const = 0;
    virtual std::string readFile(const std::filesystem::path& path) const = 0;
};

class FileSystemEnvironment : public IEnvironment {
public:
    explicit FileSystemEnvironment(std::filesystem::path directory) : directory_(std::move(directory)) {}

    std::vector<std::filesystem::path> listSkillFiles() const override;
    std::string readFile(const std::filesystem::path& path) const override;

private:
    std::filesystem::path directory_;
};

// SkillTool: a minimal `Tool` (tool.h) implementation for a single loaded
// skill. It holds a shared_ptr<SkillDescriptor> rather than a copy of the
// fields so that reloading a skill (see SkillLoader::reload) can update
// every existing SkillTool sharing that descriptor in one place.
//
// ĐỒNG BỘ HÓA: kế thừa `Tool` (tool.h) thay vì `ITool` cũ; execute() giờ
// nhận/trả về std::string (JSON) thay vì ToolArgs/ToolResult.
class SkillTool : public Tool {
public:
    explicit SkillTool(std::shared_ptr<const SkillDescriptor> descriptor) : descriptor_(std::move(descriptor)) {}

    std::string getName() const override { return descriptor_->name; }
    std::string getDescription() const override { return descriptor_->description; }
    std::string getParametersSchema() const override {
        return R"({"type": "object", "properties": {}, "required": []})";
    }
    std::string execute(const std::string& /*arguments*/) override {
        return R"({"success": true, "result": {"reply": ")" + jsonEscape(descriptor_->reply) + R"("}})";
    }

private:
    std::shared_ptr<const SkillDescriptor> descriptor_;
};

class SkillLoader {
public:
    explicit SkillLoader(std::shared_ptr<IEnvironment> env) : env_(std::move(env)) {}

    // Scans the environment for `.skill` files, parses each into a
    // SkillDescriptor, and returns one freshly-constructed
    // std::unique_ptr<Tool> per descriptor — ready to hand straight to
    // ToolRegistry::registerTool(...).
    std::vector<std::unique_ptr<Tool>> loadAll();

    // How many descriptors are currently cached (post loadAll()).
    std::size_t cachedCount() const { return descriptors_.size(); }

private:
    std::shared_ptr<IEnvironment> env_;
    // Generic Registry<T> (C++17 template class) reused here exactly as it
    // is in ToolRegistry — the descriptor cache is keyed by skill name.
    Registry<std::string, std::shared_ptr<SkillDescriptor>> descriptors_;

    static std::optional<SkillDescriptor> parse(const std::string& contents);
};
