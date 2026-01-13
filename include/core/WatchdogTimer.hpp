#pragma once

#include "Logger.hpp"
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdlib>

namespace Leviathan {
namespace Core {

/**
 * @brief Watchdog timer to detect compositor freezes and force exit
 * 
 * This prevents the compositor from completely freezing the system.
 * If the main thread doesn't "pet" the watchdog within the timeout period,
 * the watchdog will forcefully terminate the process.
 * 
 * Usage:
 *   WatchdogTimer watchdog(5);  // 5 second timeout
 *   watchdog.Start();
 *   
 *   // In main event loop:
 *   watchdog.Pet();  // Reset the timer
 *   
 *   // On shutdown:
 *   watchdog.Stop();
 */
class WatchdogTimer {
public:
    /**
     * @brief Create a watchdog timer
     * @param timeout_seconds How long to wait before force-killing (default: 10 seconds)
     */
    explicit WatchdogTimer(int timeout_seconds = 10)
        : timeout_seconds_(timeout_seconds),
          running_(false),
          last_pet_time_(std::chrono::steady_clock::now()) {}
    
    ~WatchdogTimer() {
        Stop();
    }
    
    /**
     * @brief Start the watchdog timer
     */
    void Start() {
        if (running_) return;
        
        running_ = true;
        last_pet_time_ = std::chrono::steady_clock::now();
        watchdog_thread_ = std::thread(&WatchdogTimer::WatchdogLoop, this);
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Watchdog timer started ({}s timeout)", timeout_seconds_);
    }
    
    /**
     * @brief Stop the watchdog timer (call before clean shutdown)
     */
    void Stop() {
        if (!running_) return;
        
        running_ = false;
        if (watchdog_thread_.joinable()) {
            watchdog_thread_.join();
        }
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Watchdog timer stopped");
    }
    
    /**
     * @brief Pet the watchdog (reset the timer)
     * Call this regularly from the main event loop
     */
    void Pet() {
        last_pet_time_ = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief Check if watchdog is running
     */
    bool IsRunning() const { return running_; }

private:
    void WatchdogLoop() {
        while (running_) {
            // Check every second
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            if (!running_) break;
            
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - last_pet_time_.load()
            ).count();
            
            if (elapsed >= timeout_seconds_) {
                // Compositor is frozen! Force exit
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::CRITICAL, 
                    "WATCHDOG TIMEOUT! Compositor hasn't responded in {}s. "
                    "Force terminating to prevent system freeze.",
                    elapsed
                );
                
                // Try graceful exit first
                std::raise(SIGTERM);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // If still alive, force kill
                if (running_) {
                    Leviathan::Log::WriteToLog(Leviathan::LogLevel::CRITICAL, "SIGTERM failed, sending SIGKILL");
                    std::raise(SIGKILL);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    // Last resort
                    std::abort();
                }
            }
        }
    }
    
    int timeout_seconds_;
    std::atomic<bool> running_;
    std::atomic<std::chrono::steady_clock::time_point> last_pet_time_;
    std::thread watchdog_thread_;
};

} // namespace Core
} // namespace Leviathan
