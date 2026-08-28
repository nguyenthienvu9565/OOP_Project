#include "tool.h"
#include "tool_definition.h"
#include <regex>
#include <sstream>
#include <string>
#include <vector>

// ==========================================
// TRIEN KHAI LOP REGEXSEARCHTOOL
// ==========================================

RegexSearchTool::RegexSearchTool()
    : Tool("regex_search",
           "Searches for all text fragments matching a regular expression pattern. "
           "Input format: '<pattern>|<text>' (separated by pipe '|'). "
           "Pattern uses C++ std::regex ECMAScript syntax. "
           "Example: '\\\\d{2}/\\\\d{2}/\\\\d{4}|Report date: 25/08/2026, deadline 30/08/2026' "
           "returns all dates found: ['25/08/2026', '30/08/2026']. "
           "Useful after fetch_url to extract emails, phone numbers, dates, URLs from raw text.") {}

bool RegexSearchTool::parseArguments(const std::string& input,
                                      std::string& pattern,
                                      std::string& text) {
    auto sep = input.find('|');
    if (sep == std::string::npos) {
        return false;
    }
    pattern = input.substr(0, sep);
    text    = input.substr(sep + 1);
    return true;
}

std::vector<std::string> RegexSearchTool::findAllMatches(const std::string& pattern,
                                                           const std::string& text) {
    std::regex re(pattern, std::regex::ECMAScript);
    std::vector<std::string> matches;

    auto it  = std::sregex_iterator(text.begin(), text.end(), re);
    auto end = std::sregex_iterator();

    for (; it != end; ++it) {
        matches.push_back((*it)[0].str());
    }
    return matches;
}

std::string RegexSearchTool::execute(const std::string& arguments) {
    if (arguments.empty()) {
        return "Error: Arguments are empty. "
               "Expected format: '<regex_pattern>|<text_to_search>'";
    }

    std::string pattern, text;
    if (!parseArguments(arguments, pattern, text)) {
        return "Error: Missing separator '|'. "
               "Expected format: '<regex_pattern>|<text_to_search>'. "
               "Example: '\\\\d+'|'There are 42 students and 7 teachers'";
    }

    if (pattern.empty()) {
        return "Error: Regex pattern is empty.";
    }
    if (text.empty()) {
        return "Error: Text to search is empty.";
    }

    std::vector<std::string> matches;
    try {
        matches = findAllMatches(pattern, text);
    } catch (const std::regex_error& e) {
        return "Error: Invalid regex pattern '" + pattern + "': " + std::string(e.what());
    }

    if (matches.empty()) {
        return "No matches found for pattern '" + pattern + "' in the given text.";
    }

    std::ostringstream oss;
    oss << "Found " << matches.size() << " match(es) for pattern '" << pattern << "':\n";
    for (size_t i = 0; i < matches.size(); ++i) {
        oss << "  " << (i + 1) << ". \"" << matches[i] << "\"\n";
    }
    return oss.str();
}
