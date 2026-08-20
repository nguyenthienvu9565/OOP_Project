#!/usr/bin/env bash
# Build the demo. Usage: ./build.sh
#
# The project is written to compile under C++17 at a minimum, and to
# automatically light up newer-standard code paths (C++20 concepts/<=>/
# designated init, C++23 std::expected/if consteval, C++26 pack indexing)
# on any compiler that supports them, via feature-test macros. This script
# just picks the newest -std= flag the installed g++ actually accepts.
set -e

SOURCES="main.cpp tool_registry.cpp calculator_tool.cpp skill_loader.cpp"

try_std() {
    g++ -std="$1" -fsyntax-only -x c++ - <<< "int main(){}" 2>/dev/null
}

for std in c++26 c++2c c++23 c++20 c++17; do
    if try_std "$std"; then
        echo "Using -std=$std"
        g++ -std="$std" -Wall -Wextra -O2 -o demo $SOURCES
        echo "Built ./demo (standard: $std)"
        exit 0
    fi
done

echo "No usable C++17-or-newer g++ found." >&2
exit 1
