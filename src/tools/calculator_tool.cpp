#include "tool.h"
#include "tool_definition.h"
#include <string>
#include <stack>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

CalculatorTool::CalculatorTool()
    : Tool("calculator", 
           "Evaluates a mathematical expression. "
           "Input can be a raw expression string (e.g., \"(15 * 17) + 5\") "
           "or a JSON object: {\"expression\": \"15 * 17\"}") {}

int CalculatorTool::getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

bool CalculatorTool::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

double CalculatorTool::applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0) throw std::runtime_error("Division by zero");
            return a / b;
    }
    return 0;
}

double CalculatorTool::evaluateExpression(const std::string& tokens) {
    std::stack<double> values; 
    std::stack<char> ops;   

    for (size_t i = 0; i < tokens.length(); i++) {
        if (isspace(tokens[i])) continue;

        if (tokens[i] == '(') {
            ops.push(tokens[i]);
        }
        else if (isdigit(tokens[i]) || tokens[i] == '.') {
            std::string valStr = "";
            while (i < tokens.length() && (isdigit(tokens[i]) || tokens[i] == '.')) {
                valStr += tokens[i];
                i++;
            }
            values.push(std::stod(valStr));
            i--; 
        }
        else if (tokens[i] == ')') {
            while (!ops.empty() && ops.top() != '(') {
                if (values.size() < 2) throw std::runtime_error("Malformed expression");
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(val1, val2, op));
            }
            if (!ops.empty()) ops.pop(); 
        }
        else if (isOperator(tokens[i])) {
            if (tokens[i] == '-' && (i == 0 || tokens[i-1] == '(' || isOperator(tokens[i-1]))) {
                std::string valStr = "-";
                i++;
                while (i < tokens.length() && (isdigit(tokens[i]) || tokens[i] == '.')) {
                    valStr += tokens[i];
                    i++;
                }
                if (valStr == "-") throw std::runtime_error("Invalid negative number placement");
                values.push(std::stod(valStr));
                i--;
                continue;
            }

            while (!ops.empty() && getPrecedence(ops.top()) >= getPrecedence(tokens[i])) {
                if (values.size() < 2) throw std::runtime_error("Malformed expression");
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(val1, val2, op));
            }
            ops.push(tokens[i]);
        }
        else {
            throw std::runtime_error(std::string("Unknown character: ") + tokens[i]);
        }
    }

    while (!ops.empty()) {
        if (ops.top() == '(') throw std::runtime_error("Mismatched parentheses");
        if (values.size() < 2) throw std::runtime_error("Malformed expression");
        double val2 = values.top(); values.pop();
        double val1 = values.top(); values.pop();
        char op = ops.top(); ops.pop();
        values.push(applyOp(val1, val2, op));
    }

    if (values.empty()) throw std::runtime_error("Empty expression");
    return values.top();
}

std::string CalculatorTool::execute(const std::string& arguments) {
    std::string expression = arguments;

    if (!arguments.empty() && arguments.front() == '{') {
        try {
            auto j = json::parse(arguments);
            if (j.contains("expression") && j["expression"].is_string()) {
                expression = j["expression"].get<std::string>();
            }
        } catch (const json::parse_error& e) {
            return "Error: Invalid JSON format in calculator arguments: " + std::string(e.what());
        }
    }

    if (expression.empty()) {
        return "Error: Mathematical expression is empty.";
    }

    try {
        double result = evaluateExpression(expression);

        std::ostringstream ss;
        ss << std::setprecision(10) << result;
        return ss.str();
    } 
    catch (const std::exception& e) {
        return "Error: " + std::string(e.what());
    }
}