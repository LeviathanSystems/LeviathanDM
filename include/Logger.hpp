#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <vector>

namespace Leviathan {

class Logger {
public:
    static void Init() {
        // Create console sink with colors
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        
        // Create file sink - logs to leviathan.log in current directory
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("leviathan.log", true);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
        
        // Combine both sinks
        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("leviathan", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::debug); // Auto-flush on debug and above
        spdlog::set_default_logger(logger);
        
        spdlog::info("Logger initialized - writing to console and leviathan.log");
    }
    
    static void SetLevel(spdlog::level::level_enum level) {
        spdlog::set_level(level);
    }
};

} // namespace Leviathan

// Convenience macros
#define LOG_TRACE(...)    spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...)    spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...)     spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)     spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)    spdlog::error(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)
