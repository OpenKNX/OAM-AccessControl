#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
inline std::vector<std::string> errorLogs, debugLogs;
inline void recordLog(std::vector<std::string>& logs, const char* format, ...) {
    char text[256];
    va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args);
    logs.emplace_back(text);
}
#define ESP_LOGE(tag, ...) recordLog(errorLogs, __VA_ARGS__)
#define ESP_LOGD(tag, ...) recordLog(debugLogs, __VA_ARGS__)
