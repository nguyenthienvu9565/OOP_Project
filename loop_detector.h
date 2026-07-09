#pragma once
#include <string>
#include <string_view>
#include <deque>

namespace agent 
{
    enum class LoopSeverity 
    {
        None,
        Warning,
        Critical
    };

    struct LoopDetectorConfig
    {
        int warning_threshold = 2;
        int critical_threshold = 3;
        int pingpong_length = 4;
        int history_size = 20;
    };

    class LoopDetector
    {
    private:
        LoopDetectorConfig LuuThongSoCauHinh;
        std::deque<std::string> LuuHangDoiLichSu;
        std::string LuuNguyenNhanGayLoi;

    public:
        explicit LoopDetector(LoopDetectorConfig cfg = {})
            : LuuThongSoCauHinh(cfg)
        {
        }

        void reset();
        

        [[nodiscard]] LoopSeverity record(std::string_view action_signature);
        
        const std::string& last_reason() const 
        {
            return LuuNguyenNhanGayLoi;
        }
    };
}