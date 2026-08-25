#pragma once

#include <string>
#include <string_view>

class Tool {
protected:
    std::string name;
    std::string description;

public:
    Tool(std::string_view name, std::string_view description)
        : name(name), description(description) {}

    virtual ~Tool() = default;

    [[nodiscard]] std::string getName() const { return name; }
    [[nodiscard]] std::string getDescription() const { return description; }
    virtual std::string execute(const std::string& arguments) = 0;
};