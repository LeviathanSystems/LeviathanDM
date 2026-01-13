#pragma once

#include "SimpleLogger.hpp"
#include "LogLevel.hpp"
#include <string>
#include <sstream>

namespace Leviathan {

// Unified logging interface
class Log {
public:
    // Simple message
    static void WriteToLog(LogLevel level, const std::string& message) {
        SimpleLogger::Instance().Log(level, message);
    }
    
    // Formatted message (variadic template for 1+ arguments)
    template<typename... Args>
    static void WriteToLog(LogLevel level, const std::string& format, Args&&... args) {
        SimpleLogger::Instance().Log(level, Format(format, std::forward<Args>(args)...));
    }

private:
    // Helper to convert std::string to const char* for formatting
    template<typename T>
    static auto ToFormattable(T&& value) -> decltype(std::forward<T>(value)) {
        return std::forward<T>(value);
    }
    
    static const char* ToFormattable(const std::string& str) {
        return str.c_str();
    }
    
    static const char* ToFormattable(std::string& str) {
        return str.c_str();
    }
    
    // Format helper
    static std::string Format(const std::string& format_str) {
        return format_str;
    }
    
    template<typename T, typename... Args>
    static std::string Format(const std::string& format_str, T value, Args... args) {
        size_t pos = format_str.find("{}");
        if (pos != std::string::npos) {
            std::ostringstream oss;
            oss << format_str.substr(0, pos) << ToFormattable(value);
            return oss.str() + Format(format_str.substr(pos + 2), args...);
        }
        // No more placeholders, just return format string
        return format_str;
    }
};

} // namespace Leviathan

