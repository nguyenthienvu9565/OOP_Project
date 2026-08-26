#include "tool_registry.h"
#include <memory>
#include "exec_tool.cpp"
#include "file_tool.cpp"
#include "web_tool.cpp"
#include "memory_tool.cpp"
#include "datetime_tool.cpp"
#include "fetch_url_tool.cpp"
#include "weather_tool.cpp"
#include "regex_search_tool.cpp"

/**
 * @brief Tao va tra ve mot ToolRegistry da duoc nap san toan bo tool.
 *
 * Ham factory nay tap trung toan bo viec khoi tao tool vao mot file rieng
 * de khong lam thay doi file tool_registry.h va tool_registry.cpp goc.
 *
 * @return std::unique_ptr<ToolRegistry> Registry da co du tool, ready to use
 */
std::unique_ptr<ToolRegistry> createDefaultRegistry() {
    auto registry = std::make_unique<ToolRegistry>();
    registry->registerTool(std::make_unique<ExecTool>());
    registry->registerTool(std::make_unique<WebSearchTool>());
    registry->registerTool(std::make_unique<MemorySaveTool>());
    registry->registerTool(std::make_unique<MemorySearchTool>());
    registry->registerTool(std::make_unique<DateTimeTool>());      
    registry->registerTool(std::make_unique<FetchUrlTool>());      
    registry->registerTool(std::make_unique<WeatherTool>());       
    registry->registerTool(std::make_unique<RegexSearchTool>());   
    return registry;
}
