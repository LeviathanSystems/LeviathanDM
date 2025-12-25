#pragma once

#include "LogLevel.hpp"
#include "LogSink.hpp"
#include <string>
#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <atomic>

namespace Leviathan {

class SimpleLogger {
public:
    static SimpleLogger& Instance() {
        static SimpleLogger instance;
        return instance;
    }

    void Init(const std::string& log_file = "leviathan.log", 
              LogLevel level = LogLevel::DEBUG,
              bool console_colors = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return;
        
        min_level_ = level;
        
        // Add console sink
        auto console_sink = std::make_shared<ConsoleSink>(console_colors);
        console_sink->SetLevel(level);
        sinks_.push_back(console_sink);
        
        // Add file sink
        auto file_sink = std::make_shared<FileSink>(log_file);
        file_sink->SetLevel(level);
        sinks_.push_back(file_sink);
        
        // Start worker thread
        running_ = true;
        worker_thread_ = std::thread(&SimpleLogger::WorkerLoop, this);
        initialized_ = true;
        
        // Log initialization message and force immediate flush
        std::string init_msg = "Logger initialized with " + 
            std::to_string(sinks_.size()) + " sink(s): console + " + log_file;
        
        // Format the message
        auto now = std::chrono::system_clock::now();
        auto now_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm_buf;
        localtime_r(&now_t, &tm_buf);
        
        std::ostringstream oss;
        oss << "[" 
            << std::setfill('0') << std::setw(2) << tm_buf.tm_hour << ":"
            << std::setfill('0') << std::setw(2) << tm_buf.tm_min << ":"
            << std::setfill('0') << std::setw(2) << tm_buf.tm_sec << "."
            << std::setfill('0') << std::setw(3) << now_ms.count()
            << "] [" << LevelToString(LogLevel::INFO) << "] "
            << init_msg;
        
        std::string log_line = oss.str();
        
        // Write directly to all sinks (bypass queue for init message)
        for (auto& sink : sinks_) {
            if (sink->ShouldLog(LogLevel::INFO)) {
                sink->Write(LogLevel::INFO, log_line);
                sink->Flush();  // Immediate flush
            }
        }
    }
    
    void AddSink(std::shared_ptr<LogSink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.push_back(sink);
    }

    void Shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!initialized_) return;
            running_ = false;
        }
        cv_.notify_one();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        
        sinks_.clear();
        initialized_ = false;
    }

    void SetLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        min_level_ = level;
        for (auto& sink : sinks_) {
            sink->SetLevel(level);
        }
    }

    void Log(LogLevel level, const std::string& message) {
        if (level < min_level_) return;
        
        auto now = std::chrono::system_clock::now();
        auto now_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm_buf;
        localtime_r(&now_t, &tm_buf);
        
        std::ostringstream oss;
        
        // Console format: [HH:MM:SS.mmm] [level] message
        oss << "[" 
            << std::setfill('0') << std::setw(2) << tm_buf.tm_hour << ":"
            << std::setfill('0') << std::setw(2) << tm_buf.tm_min << ":"
            << std::setfill('0') << std::setw(2) << tm_buf.tm_sec << "."
            << std::setfill('0') << std::setw(3) << now_ms.count()
            << "] [" << LevelToString(level) << "] "
            << message;
        
        std::string log_line = oss.str();
        
        // Queue for both console and file writing (async to avoid blocking)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            log_queue_.push(std::make_pair(level, log_line));
        }
        cv_.notify_one();
    }

    ~SimpleLogger() {
        Shutdown();
    }

private:
    SimpleLogger() : initialized_(false), running_(false), min_level_(LogLevel::DEBUG) {}
    
    SimpleLogger(const SimpleLogger&) = delete;
    SimpleLogger& operator=(const SimpleLogger&) = delete;

    void WorkerLoop() {
        while (running_) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !log_queue_.empty() || !running_; });
            
            // Process batch of messages
            std::vector<std::pair<LogLevel, std::string>> batch;
            while (!log_queue_.empty() && batch.size() < 100) {
                batch.push_back(log_queue_.front());
                log_queue_.pop();
            }
            lock.unlock();
            
            // Write to all sinks
            for (const auto& [level, message] : batch) {
                for (auto& sink : sinks_) {
                    if (sink->ShouldLog(level)) {
                        sink->Write(level, message);
                    }
                }
            }
            
            // Flush all sinks periodically
            for (auto& sink : sinks_) {
                sink->Flush();
            }
        }
        
        // Flush remaining messages on shutdown
        std::lock_guard<std::mutex> lock(mutex_);
        while (!log_queue_.empty()) {
            auto [level, message] = log_queue_.front();
            for (auto& sink : sinks_) {
                if (sink->ShouldLog(level)) {
                    sink->Write(level, message);
                }
            }
            log_queue_.pop();
        }
        for (auto& sink : sinks_) {
            sink->Flush();
        }
    }

    const char* LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "trace";
            case LogLevel::DEBUG: return "debug";
            case LogLevel::INFO: return "info";
            case LogLevel::WARN: return "warn";
            case LogLevel::ERROR: return "error";
            case LogLevel::CRITICAL: return "critical";
            default: return "unknown";
        }
    }

    bool initialized_;
    std::atomic<bool> running_;
    LogLevel min_level_;
    
    std::vector<std::shared_ptr<LogSink>> sinks_;
    std::queue<std::pair<LogLevel, std::string>> log_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
};

} // namespace Leviathan

// Convenience macros with basic string formatting
#define LOG_TRACE(msg) Leviathan::SimpleLogger::Instance().Log(Leviathan::LogLevel::TRACE, msg)
#define LOG_DEBUG(msg) Leviathan::SimpleLogger::Instance().Log(Leviathan::LogLevel::DEBUG, msg)
#define LOG_INFO(msg) Leviathan::SimpleLogger::Instance().Log(Leviathan::LogLevel::INFO, msg)
#define LOG_WARN(msg) Leviathan::SimpleLogger::Instance().Log(Leviathan::LogLevel::WARN, msg)
#define LOG_ERROR(msg) Leviathan::SimpleLogger::Instance().Log(Leviathan::LogLevel::ERROR, msg)
#define LOG_CRITICAL(msg) Leviathan::SimpleLogger::Instance().Log(Leviathan::LogLevel::CRITICAL, msg)
