cmake_minimum_required(VERSION 3.20)
project(OOP_Project CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Homebrew & System header search paths
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    /opt/homebrew/include
    /usr/local/include
)

link_directories(
    /opt/homebrew/lib
    /usr/local/lib
)

# Source files across all 6 layers
set(PROJECT_SOURCES
    src/LLM_Client/LLM_Client.cpp
    src/tools/tool_registry.cpp
    src/tools/calculator_tool.cpp
    src/tools/datetime_tool.cpp
    src/tools/exec_tool.cpp
    src/tools/fetch_url_tool.cpp
    src/tools/file_tool.cpp
    src/tools/memory_tool.cpp
    src/tools/regex_search_tool.cpp
    src/tools/weather_tool.cpp
    src/tools/web_tool.cpp
    src/skills/skill_loader.cpp
    src/agent/agent_loop.cpp
    src/agent/loop_detector.cpp
    src/harness/evaluator.cpp
    src/harness/harness.cpp
)

# Executable for Layer 1-6 Integration & Benchmark
add_executable(run_eval benchmark/run_eval.cpp ${PROJECT_SOURCES})

# Find and link cpr, curl, and sqlite3
target_link_libraries(run_eval PRIVATE cpr curl sqlite3)
