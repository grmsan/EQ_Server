#include "stat_debug.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <thread>

static std::mutex stat_log_mutex;

namespace StatDebug {
    static std::string Timestamp() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream ss;
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        ss << std::put_time(&tm, "%Y%m%d_%H%M%S") << '_' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    void LogMessage(const char* file, const char* function, int line, const std::string &msg) {
        std::lock_guard<std::mutex> guard(stat_log_mutex);
        std::ofstream ofs("logs/stat_trace.log", std::ios::app);
        if(!ofs)
            return;

        std::ostringstream ss;
        ss << Timestamp() << " ";
        ss << "pid=" << std::this_thread::get_id() << " ";
        ss << file << ":" << function << ":" << line << " ";
        ss << msg;

        ofs << ss.str() << "\n";
        ofs.flush();
    }
}
