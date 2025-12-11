#pragma once

#include <string>

namespace StatDebug {
    void LogMessage(const char* file, const char* function, int line, const std::string &msg);
}

#define STAT_LOG(msg) StatDebug::LogMessage(__FILE__, __FUNCTION__, __LINE__, (msg))
