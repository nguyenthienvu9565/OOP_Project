#include "loop_detector.h"

namespace agent 
{
    void LoopDetector::reset()
    {
        LuuHangDoiLichSu.clear();
        LuuNguyenNhanGayLoi.clear();
    }

    LoopSeverity LoopDetector::record(std::string_view action_signature)
    {
        LuuHangDoiLichSu.push_back(std::string(action_signature));
        if (LuuHangDoiLichSu.size() > LuuThongSoCauHinh.history_size)
        {
            LuuHangDoiLichSu.pop_front();
        }
        
        size_t N = LuuHangDoiLichSu.size();

        if (N >= LuuThongSoCauHinh.pingpong_length)
        {
            if ((LuuHangDoiLichSu[N-1] == LuuHangDoiLichSu[N-3]) && (LuuHangDoiLichSu[N-2] == LuuHangDoiLichSu[N-4]))
            {
                LuuNguyenNhanGayLoi = "Agent bi lap lai.";
                return LoopSeverity::Critical;
            } 
        }

        int dem_lien_tiep = 0;
        for (int i = N - 1; i >= 0; i--)
        {
            if (LuuHangDoiLichSu[i] == LuuHangDoiLichSu.back())
            {
                dem_lien_tiep++;
            }
            else
            {
                break;
            }
        }

        if (dem_lien_tiep >= LuuThongSoCauHinh.critical_threshold)
        {
            LuuNguyenNhanGayLoi = "Agent bi lap lien tiep.";
            return LoopSeverity::Critical;
        }
        else if (dem_lien_tiep >= LuuThongSoCauHinh.warning_threshold)
        {
            return LoopSeverity::Warning;
        }

        return LoopSeverity::None;
    }
}