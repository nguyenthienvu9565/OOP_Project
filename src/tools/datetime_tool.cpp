#pragma once

#include "tool.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <cctype>

/**
 * @brief Tool ho tro LLM lay ngay gio hien tai cua he thong.
 * Ten dang ky: "get_datetime"
 */
class DateTimeTool : public Tool {
public:
    DateTimeTool();
    ~DateTimeTool() override = default;

    /**
     * @brief Thuc thi viec lay ngay gio.
     * @param arguments Dinh dang mong muon ("full", "date", "time", "iso")
     * @return Chuoi ngay gio he thong da dinh dang.
     */
    std::string execute(const std::string& arguments) override;

private:
    // Lay thoi gian cuc bo he thong (an toan da luong)
    static std::tm getLocalTime();
};

// ==========================================
// TRIEN KHAI LOP DATETIMETOOL
// ==========================================

DateTimeTool::DateTimeTool()
    : Tool("get_datetime",
           "Returns the current system date and time in local timezone. "
           "Use this whenever you need to know the current date or time. "
           "Optional argument: 'full' (default, e.g. 'Mon, 25 Aug 2026 14:48:17'), "
           "'date' (e.g. '2026-08-25'), 'time' (e.g. '14:48:17'), "
           "'iso' (ISO 8601, e.g. '2026-08-25T14:48:17'). "
           "Example input: 'date'") {}

std::tm DateTimeTool::getLocalTime() {
    auto now       = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    return local_tm;
}

std::string DateTimeTool::execute(const std::string& arguments) {
    std::tm local_tm = getLocalTime();
    std::ostringstream oss;

    // Chuan hoa tham so: xoa khoang trang va chuyen ve chu thuong
    std::string fmt = arguments;
    auto start = fmt.find_first_not_of(" \t\r\n");
    auto end   = fmt.find_last_not_of(" \t\r\n");
    fmt        = (start == std::string::npos) ? "" : fmt.substr(start, end - start + 1);
    for (char& c : fmt) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (fmt == "date") {
        oss << std::put_time(&local_tm, "%Y-%m-%d");
    } else if (fmt == "time") {
        oss << std::put_time(&local_tm, "%H:%M:%S");
    } else if (fmt == "iso") {
        oss << std::put_time(&local_tm, "%Y-%m-%dT%H:%M:%S");
    } else {
        oss << std::put_time(&local_tm, "%a, %d %b %Y %H:%M:%S (local time)");
    }

    return oss.str();
}
