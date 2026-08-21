// calculator_tool.cpp
#include "calculator_tool.hpp"
#include "json_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>

// ---------------------------------------------------------------------
// small string helpers
// ---------------------------------------------------------------------
namespace {

std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
    return s.substr(a, b - a);
}

bool isIdentifier(const std::string& s) {
    if (s.empty()) return false;
    if (!std::isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_') return false;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
}

// Finds a top-level '=' that represents assignment (as opposed to ==, <=,
// >=, !=). Returns std::string::npos if none is found.
size_t findAssignmentEq(const std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '=') continue;
        char prev = i > 0 ? s[i - 1] : '\0';
        char next = i + 1 < s.size() ? s[i + 1] : '\0';
        if (next == '=') continue;                              // "=="
        if (prev == '=' || prev == '<' || prev == '>' || prev == '!') continue;  // "<=", ">=", "!="
        return i;
    }
    return std::string::npos;
}

// -----------------------------------------------------------------------
// consteval / if consteval (C++23): a single function body that behaves
// differently depending on whether it is being evaluated at compile time
// or at run time. At compile time we hand-fold the digits of pi so the
// constant is provably independent of <cmath>'s runtime M_PI; at run time
// we just use M_PI directly (cheaper: no need to re-derive anything).
// Either branch produces the same degrees->radians factor.
// -----------------------------------------------------------------------
constexpr double degToRadFactor() {
    if consteval {
        constexpr double compileTimePi = 3.14159265358979323846;
        return compileTimePi / 180.0;
    } else {
        return M_PI / 180.0;
    }
}

// A compile-time sanity check that the consteval branch above is actually
// reachable and correct — this line only compiles if degToRadFactor() can
// be evaluated in a constant expression.
static_assert(degToRadFactor() > 0.017453292 && degToRadFactor() < 0.017453293,
              "degToRadFactor() compile-time branch produced an unexpected value");

// -----------------------------------------------------------------------
// Evaluator: a hand-written recursive-descent parser + evaluator.
//
// This is the C++ analogue of the Python `ast`-based SafeEvaluator: instead
// of calling a scripting engine or eval(), the grammar below is the ONLY
// thing the expression string can do. There is no path from an expression
// string to arbitrary code execution.
//
// Grammar (highest to lowest precedence):
//   primary    := NUMBER | IDENT | IDENT '(' args ')' | '(' expr ')'
//   power      := primary ('^' | '**') unary        (right-associative)
//   unary      := ('+' | '-') unary | power
//   mulDiv     := unary (('*' | '/' | '//' | '%') unary)*
//   addSub     := mulDiv (('+' | '-') mulDiv)*
// -----------------------------------------------------------------------
class Evaluator {
public:
    Evaluator(const std::string& expr, const std::map<std::string, double>& vars, bool degreeMode)
        : src_(expr), pos_(0), vars_(vars), degree_(degreeMode) {
        constants_ = {
            {"pi", M_PI},
            {"e", M_E},
            {"tau", 2.0 * M_PI},
            {"inf", std::numeric_limits<double>::infinity()},
        };
    }

    double evaluate() {
        double v = parseAddSub();
        skipSpaces();
        if (pos_ != src_.size()) {
            throw CalculatorError("Cú pháp không hợp lệ gần vị trí " + std::to_string(pos_) +
                                   " (invalid syntax)");
        }
        return v;
    }

private:
    std::string src_;
    size_t pos_;
    const std::map<std::string, double>& vars_;
    bool degree_;
    std::map<std::string, double> constants_;

    void skipSpaces() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) pos_++;
    }

    bool consume(char c) {
        skipSpaces();
        if (pos_ < src_.size() && src_[pos_] == c) {
            pos_++;
            return true;
        }
        return false;
    }

    bool consumeStr(const std::string& s) {
        skipSpaces();
        if (src_.compare(pos_, s.size(), s) == 0) {
            pos_ += s.size();
            return true;
        }
        return false;
    }

    double parseAddSub() {
        double v = parseMulDiv();
        while (true) {
            skipSpaces();
            if (consume('+')) {
                v += parseMulDiv();
            } else if (consume('-')) {
                v -= parseMulDiv();
            } else {
                break;
            }
        }
        return v;
    }

    double parseMulDiv() {
        double v = parseUnary();
        while (true) {
            skipSpaces();
            if (pos_ + 1 < src_.size() && src_[pos_] == '/' && src_[pos_ + 1] == '/') {
                pos_ += 2;
                double rhs = parseUnary();
                if (rhs == 0) throw CalculatorError("Lỗi: chia cho 0 (division by zero).");
                v = std::floor(v / rhs);
            } else if (consume('*')) {
                v *= parseUnary();
            } else if (consume('/')) {
                double rhs = parseUnary();
                if (rhs == 0) throw CalculatorError("Lỗi: chia cho 0 (division by zero).");
                v /= rhs;
            } else if (consume('%')) {
                double rhs = parseUnary();
                if (rhs == 0) throw CalculatorError("Lỗi: chia cho 0 (division by zero).");
                v = std::fmod(v, rhs);
            } else {
                break;
            }
        }
        return v;
    }

    double parseUnary() {
        skipSpaces();
        if (consume('+')) return parseUnary();
        if (consume('-')) return -parseUnary();
        return parsePower();
    }

    double parsePower() {
        double base = parsePrimary();
        skipSpaces();
        if (consumeStr("**") || consume('^')) {
            double exponent = parseUnary();  // right-associative: 2^3^2 == 2^(3^2)
            return std::pow(base, exponent);
        }
        return base;
    }

    double parsePrimary() {
        skipSpaces();
        if (consume('(')) {
            double v = parseAddSub();
            if (!consume(')')) throw CalculatorError("Thiếu dấu ')' (missing closing parenthesis)");
            return v;
        }
        if (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.')) {
            return parseNumber();
        }
        if (pos_ < src_.size() &&
            (std::isalpha(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) {
            return parseIdentifierOrCall();
        }
        throw CalculatorError("Biểu thức không hợp lệ gần vị trí " + std::to_string(pos_));
    }

    double parseNumber() {
        size_t start = pos_;
        while (pos_ < src_.size() &&
               (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.')) {
            pos_++;
        }
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            size_t save = pos_;
            pos_++;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) pos_++;
            if (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) pos_++;
            } else {
                pos_ = save;  // not actually an exponent, e.g. a bare "e" constant use elsewhere
            }
        }
        std::string numStr = src_.substr(start, pos_ - start);
        try {
            return std::stod(numStr);
        } catch (...) {
            throw CalculatorError("Số không hợp lệ: '" + numStr + "'");
        }
    }

    std::string parseIdentifier() {
        size_t start = pos_;
        while (pos_ < src_.size() &&
               (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) {
            pos_++;
        }
        return src_.substr(start, pos_ - start);
    }

    double parseIdentifierOrCall() {
        std::string ident = parseIdentifier();
        skipSpaces();
        if (consume('(')) {
            std::vector<double> args;
            skipSpaces();
            if (!consume(')')) {
                while (true) {
                    args.push_back(parseAddSub());
                    skipSpaces();
                    if (consume(',')) continue;
                    if (consume(')')) break;
                    throw CalculatorError("Thiếu dấu ',' hoặc ')' trong lời gọi hàm '" + ident + "'");
                }
            }
            return callFunction(ident, args);
        }
        if (constants_.count(ident)) return constants_.at(ident);
        if (vars_.count(ident)) return vars_.at(ident);
        throw CalculatorError("Biến không xác định: '" + ident + "'");
    }

    // Degree/radian conversion now goes through degToRadFactor() (if
    // consteval, C++23) instead of hand-writing "M_PI / 180.0" twice.
    double toRad(double x) const { return degree_ ? x * degToRadFactor() : x; }
    double fromRad(double x) const { return degree_ ? x / degToRadFactor() : x; }

    double callFunction(const std::string& name, const std::vector<double>& a) {
        auto need = [&](size_t n) {
            if (a.size() != n) {
                throw CalculatorError("Hàm '" + name + "' cần " + std::to_string(n) + " tham số");
            }
        };

        if (name == "sqrt") {
            need(1);
            if (a[0] < 0) throw CalculatorError("Không thể lấy căn bậc hai của số âm");
            return std::sqrt(a[0]);
        }
        if (name == "abs") { need(1); return std::fabs(a[0]); }
        if (name == "round") { need(1); return std::round(a[0]); }
        if (name == "floor") { need(1); return std::floor(a[0]); }
        if (name == "ceil") { need(1); return std::ceil(a[0]); }
        if (name == "exp") { need(1); return std::exp(a[0]); }
        if (name == "log10") { need(1); return std::log10(a[0]); }
        if (name == "log2") { need(1); return std::log2(a[0]); }
        if (name == "log") {
            if (a.size() == 1) return std::log(a[0]);
            if (a.size() == 2) return std::log(a[0]) / std::log(a[1]);
            throw CalculatorError("Hàm 'log' cần 1 hoặc 2 tham số");
        }
        if (name == "sin") { need(1); return std::sin(toRad(a[0])); }
        if (name == "cos") { need(1); return std::cos(toRad(a[0])); }
        if (name == "tan") { need(1); return std::tan(toRad(a[0])); }
        if (name == "asin") { need(1); return fromRad(std::asin(a[0])); }
        if (name == "acos") { need(1); return fromRad(std::acos(a[0])); }
        if (name == "atan") { need(1); return fromRad(std::atan(a[0])); }
        if (name == "factorial") {
            need(1);
            if (a[0] < 0 || a[0] != std::floor(a[0])) {
                throw CalculatorError("factorial chỉ áp dụng cho số nguyên không âm");
            }
            double result = 1;
            for (int i = 2; i <= static_cast<int>(a[0]); ++i) result *= i;
            return result;
        }
        if (name == "min") {
            if (a.empty()) throw CalculatorError("'min' cần ít nhất 1 tham số");
            return *std::min_element(a.begin(), a.end());
        }
        if (name == "max") {
            if (a.empty()) throw CalculatorError("'max' cần ít nhất 1 tham số");
            return *std::max_element(a.begin(), a.end());
        }
        if (name == "sum") {
            double s = 0;
            for (double x : a) s += x;
            return s;
        }
        if (name == "mean") {
            if (a.empty()) throw CalculatorError("'mean' cần ít nhất 1 tham số");
            double s = 0;
            for (double x : a) s += x;
            return s / static_cast<double>(a.size());
        }
        if (name == "pow") { need(2); return std::pow(a[0], a[1]); }
        if (name == "gcd") {
            need(2);
            long long x = static_cast<long long>(a[0]);
            long long y = static_cast<long long>(a[1]);
            while (y != 0) {
                long long t = y;
                y = x % y;
                x = t;
            }
            return static_cast<double>(std::llabs(x));
        }

        throw CalculatorError("Hàm không hỗ trợ: '" + name + "'");
    }
};

}  // namespace

// ---------------------------------------------------------------------
// CalculatorTool
// ---------------------------------------------------------------------
const std::string CalculatorTool::kName = "calculator";

std::string CalculatorTool::description() {
    return "Tính toán biểu thức số học và khoa học (calculates numerical / "
           "scientific expressions). Hỗ trợ các phép +,-,*,/,//,%,**; các hàm "
           "sqrt, sin, cos, tan, asin, acos, atan, log, log2, log10, exp, "
           "factorial, floor, ceil, abs, round, min, max, sum, mean, gcd, pow; "
           "các hằng số pi, e, tau, inf. Có thể gán biến bằng cú pháp "
           "'x = 12 / 4' rồi dùng lại 'x' ở các lần gọi sau trong cùng phiên. "
           "Biến 'ans' luôn tự động giữ kết quả của lần tính gần nhất. "
           "Dùng tool này bất cứ khi nào cần một con số chính xác thay vì tự "
           "nhẩm tính, đặc biệt với số lớn, phân số, hoặc hàm khoa học.";
}

std::string CalculatorTool::parametersSchema() {
    return R"({
  "type": "object",
  "properties": {
    "expression": {
      "type": "string",
      "description": "Biểu thức cần tính. Ví dụ: '2 + 3 * (4 - 1)', 'sqrt(16) + sin(30)', 'x = 12 / 4', '(1 + ans) * 2'"
    },
    "mode": {
      "type": "string",
      "enum": ["radian", "degree"],
      "description": "Chế độ lượng giác. Mặc định 'radian'."
    },
    "action": {
      "type": "string",
      "enum": ["evaluate", "history", "clear"],
      "description": "'evaluate' (mặc định): tính biểu thức. 'history': trả về các phép tính gần đây. 'clear': xóa biến nhớ và lịch sử."
    }
  },
  "required": ["expression"]
})";
}

std::optional<double> CalculatorTool::lookupVariable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it == variables_.end()) return std::nullopt;
    return it->second;
}

std::expected<double, std::string> CalculatorTool::safeEvaluate(
    const std::string& expr, const std::map<std::string, double>& scope, bool degreeMode) {
    try {
        Evaluator evaluator(expr, scope, degreeMode);
        return evaluator.evaluate();
    } catch (const CalculatorError& e) {
        return std::unexpected(std::string(e.what()));
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Loi khong xac dinh: ") + e.what());
    }
}

CalcResult CalculatorTool::handleClear() {
    variables_.clear();
    history_.clear();
    CalcResult r;
    r.success = true;
    r.result_json = R"({"message": "Da xoa bo nho va lich su tinh toan."})";
    return r;
}

CalcResult CalculatorTool::handleHistory() {
    std::ostringstream oss;
    oss << "{\"history\": [";
    size_t start = history_.size() > 20 ? history_.size() - 20 : 0;
    for (size_t i = start; i < history_.size(); ++i) {
        if (i > start) oss << ", ";
        // Structured bindings aren't needed here (HistoryEntry has named
        // fields already), but the entries are directly comparable now
        // thanks to the defaulted operator<=> in calculator_tool.hpp.
        const HistoryEntry& entry = history_[i];
        oss << "{\"expression\": \"" << jsonEscape(entry.expression) << "\", "
            << "\"result\": " << formatNumber(entry.result) << ", "
            << "\"mode\": \"" << jsonEscape(entry.mode) << "\"}";
    }
    oss << "]}";

    CalcResult r;
    r.success = true;
    r.result_json = oss.str();
    return r;
}

CalcResult CalculatorTool::handleEvaluate(const std::string& rawExpression, const std::string& mode) {
    CalcResult r;
    std::string expr = trim(rawExpression);
    if (expr.empty()) {
        r.success = false;
        r.error = "Bieu thuc rong.";
        return r;
    }

    // Optional variable assignment: "x = <expr>"
    std::string varName;
    size_t eq = findAssignmentEq(expr);
    if (eq != std::string::npos) {
        std::string left = trim(expr.substr(0, eq));
        std::string right = trim(expr.substr(eq + 1));
        if (isIdentifier(left)) {
            varName = left;
            expr = right;
        }
    }

    std::map<std::string, double> scope = variables_;
    // std::optional<T> (C++17) in action: "ans" may or may not exist yet in
    // variables_; lookupVariable() makes that explicit instead of relying
    // on operator[] silently default-constructing a 0.0 entry.
    if (!lookupVariable("ans").has_value()) scope["ans"] = 0.0;

    // std::expected<T,E> (C++23) in action: no try/catch at this call site
    // at all — the failure path is just another value to branch on.
    std::expected<double, std::string> outcome = safeEvaluate(expr, scope, mode == "degree");
    if (!outcome.has_value()) {
        r.success = false;
        r.error = outcome.error();
        return r;
    }
    double result = outcome.value();

    variables_["ans"] = result;
    if (!varName.empty()) variables_[varName] = result;

    history_.push_back({rawExpression, result, mode});

    std::ostringstream oss;
    oss << "{\"result\": " << formatNumber(result) << ", "
        << "\"assigned_to\": " << (varName.empty() ? "null" : ("\"" + jsonEscape(varName) + "\"")) << ", "
        << "\"mode\": \"" << jsonEscape(mode) << "\"}";

    r.success = true;
    r.result_json = oss.str();
    return r;
}

// ĐỒNG BỘ HÓA: `arguments` giờ là một chuỗi JSON phẳng (được gửi xuống bởi
// ToolRegistry::executeTool của nhóm) thay vì ToolArgs map cũ — parse nó
// bằng parseFlatJsonObject() (json_utils.hpp), chạy logic y hệt như cũ, rồi
// gói CalcResult lại thành MỘT chuỗi JSON duy nhất để trả về, vì
// Tool::execute() (tool.h) chỉ trả std::string chứ không có success/error
// riêng biệt như ToolResult cũ.
std::string CalculatorTool::execute(const std::string& arguments) {
    std::map<std::string, std::string> args = parseFlatJsonObject(arguments);

    std::string action = args.count("action") ? args.at("action") : "evaluate";
    std::string mode = args.count("mode") ? args.at("mode") : "radian";
    std::string expression = args.count("expression") ? args.at("expression") : "";

    CalcResult r;
    if (action == "clear") {
        r = handleClear();
    } else if (action == "history") {
        r = handleHistory();
    } else {
        r = handleEvaluate(expression, mode);
    }

    std::ostringstream oss;
    if (r.success) {
        oss << "{\"success\": true, \"result\": " << r.result_json << "}";
    } else {
        oss << "{\"success\": false, \"error\": \"" << jsonEscape(r.error) << "\"}";
    }
    return oss.str();
}

// Lưu ý: registerInto(ToolRegistry&) đã bị loại bỏ. Với ToolRegistry của
// nhóm, đăng ký chỉ còn một dòng ở call site:
//     registry.registerTool(std::make_unique<CalculatorTool>());
// vì CalculatorTool tự khai báo tên/mô tả/schema của chính nó qua các hàm
// ảo getName()/getDescription()/getParametersSchema() — không cần
// firstOf(...) chọn "tên chính tắc" ở đây nữa (registerTool nhận thẳng
// unique_ptr<Tool>, không nhận name/description/schema rời như trước).
