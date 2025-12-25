#pragma once

#include "LogLevel.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <mutex>

namespace Leviathan {

// Abstract base class for log sinks
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void Write(LogLevel level, const std::string& message) = 0;
    virtual void Flush() = 0;
    
    void SetLevel(LogLevel level) { min_level_ = level; }
    bool ShouldLog(LogLevel level) const { return level >= min_level_; }

protected:
    LogLevel min_level_ = LogLevel::DEBUG;
};

// Console sink with color support
class ConsoleSink : public LogSink {
public:
    ConsoleSink(bool use_colors = true) : use_colors_(use_colors) {}
    
    void Write(LogLevel level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ShouldLog(level)) return;
        
        if (use_colors_) {
            std::cout << GetColorCode(level) << message << "\033[0m\n";
        } else {
            std::cout << message << "\n";
        }
    }
    
    void Flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout.flush();
    }

private:
    const char* GetColorCode(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "\033[37m";      // White
            case LogLevel::DEBUG: return "\033[36m";      // Cyan
            case LogLevel::INFO: return "\033[32m";       // Green
            case LogLevel::WARN: return "\033[33m";       // Yellow
            case LogLevel::ERROR: return "\033[31m";      // Red
            case LogLevel::CRITICAL: return "\033[1;31m"; // Bold Red
            default: return "\033[0m";
        }
    }
    
    bool use_colors_;
    std::mutex mutex_;
};

// File sink
class FileSink : public LogSink {
public:
    FileSink(const std::string& filename) : filename_(filename) {
        file_stream_.open(filename, std::ios::app);
        if (!file_stream_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }
    
    ~FileSink() override {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
    }
    
    void Write(LogLevel level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ShouldLog(level)) return;
        
        if (file_stream_.is_open()) {
            file_stream_ << message << "\n";
        }
    }
    
    void Flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_stream_.is_open()) {
            file_stream_.flush();
        }
    }
    
    bool IsOpen() const { return file_stream_.is_open(); }

private:
    std::string filename_;
    std::ofstream file_stream_;
    std::mutex mutex_;
};

} // namespace Leviathan
