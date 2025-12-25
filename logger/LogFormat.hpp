#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace Leviathan {

// Simple string formatting utility
class LogFormat {
public:
    template<typename... Args>
    static std::string Format(const std::string& format_str, Args... args) {
        std::ostringstream oss;
        FormatImpl(oss, format_str, args...);
        return oss.str();
    }

private:
    // Base case: no more arguments
    static void FormatImpl(std::ostringstream& oss, const std::string& format_str) {
        oss << format_str;
    }

    // Recursive case: process one argument at a time
    template<typename T, typename... Args>
    static void FormatImpl(std::ostringstream& oss, const std::string& format_str, 
                          T value, Args... args) {
        size_t pos = format_str.find("{}");
        if (pos != std::string::npos) {
            oss << format_str.substr(0, pos) << value;
            FormatImpl(oss, format_str.substr(pos + 2), args...);
        } else {
            oss << format_str;
        }
    }
};

} // namespace Leviathan

// Enhanced macros with formatting support
#define LOG_TRACE_FMT(fmt, ...) LOG_TRACE(Leviathan::LogFormat::Format(fmt, __VA_ARGS__))
#define LOG_DEBUG_FMT(fmt, ...) LOG_DEBUG(Leviathan::LogFormat::Format(fmt, __VA_ARGS__))
#define LOG_INFO_FMT(fmt, ...) LOG_INFO(Leviathan::LogFormat::Format(fmt, __VA_ARGS__))
#define LOG_WARN_FMT(fmt, ...) LOG_WARN(Leviathan::LogFormat::Format(fmt, __VA_ARGS__))
#define LOG_ERROR_FMT(fmt, ...) LOG_ERROR(Leviathan::LogFormat::Format(fmt, __VA_ARGS__))
#define LOG_CRITICAL_FMT(fmt, ...) LOG_CRITICAL(Leviathan::LogFormat::Format(fmt, __VA_ARGS__))
