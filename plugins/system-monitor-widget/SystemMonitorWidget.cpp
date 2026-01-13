#include "ui/PeriodicWidget.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include "ui/reusable-widgets/HBox.hpp"
#include "ui/reusable-widgets/VBox.hpp"
#include "version.h"
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
#include <cstring>
#include <iomanip>

namespace Leviathan {
namespace UI {

class SystemMonitorWidget : public PeriodicWidget {
public:
    SystemMonitorWidget() 
        : cpuUsage(0.0),
          showCPU(true),
          showMemory(true),
          showSwap(true),
          showDisks(false),
          container(nullptr),
          cpuLabel(nullptr),
          memLabel(nullptr),
          swapLabel(nullptr) {
        memset(&prevCPUStats, 0, sizeof(CPUStats));
        memset(&memInfo, 0, sizeof(MemoryInfo));
    }
    
    PluginMetadata GetMetadata() const override {
        return PluginMetadata{
            .name = PLUGIN_NAME,
            .version = PLUGIN_VERSION,
            .author = "LeviathanDM",
            .description = "Configurable system monitor (CPU, Memory, Swap, Disk)",
            .api_version = WIDGET_API_VERSION
        };
    }

protected:
    bool InitializeImpl(const std::map<std::string, std::string>& config) override {
        // Parse configuration
        auto cpu_it = config.find("show_cpu");
        if (cpu_it != config.end()) {
            showCPU = (cpu_it->second == "1" || cpu_it->second == "true");
        }
        
        auto mem_it = config.find("show_memory");
        if (mem_it != config.end()) {
            showMemory = (mem_it->second == "1" || mem_it->second == "true");
        }
        
        auto swap_it = config.find("show_swap");
        if (swap_it != config.end()) {
            showSwap = (swap_it->second == "1" || swap_it->second == "true");
        }
        
        auto disk_it = config.find("show_disks");
        if (disk_it != config.end()) {
            showDisks = (disk_it->second == "1" || disk_it->second == "true");
        }
        
        // Parse disk mounts (comma-separated)
        auto mounts_it = config.find("disk_mounts");
        if (mounts_it != config.end()) {
            std::istringstream iss(mounts_it->second);
            std::string mount;
            while (std::getline(iss, mount, ',')) {
                mount.erase(0, mount.find_first_not_of(" \t"));
                mount.erase(mount.find_last_not_of(" \t") + 1);
                if (!mount.empty()) {
                    diskMounts.push_back(mount);
                }
            }
        }
        
        if (diskMounts.empty()) {
            diskMounts.push_back("/");
        }
        
        // Create UI layout
        container = std::make_shared<HBox>();
        
        // Add CPU label
        if (showCPU) {
            cpuLabel = std::make_shared<Label>("CPU:--");
            cpuLabel->SetFontSize(font_size_);
            cpuLabel->SetTextColor(text_color_[0], text_color_[1], text_color_[2], text_color_[3]);
            container->AddChild(cpuLabel);
        }
        
        // Add Memory label
        if (showMemory) {
            memLabel = std::make_shared<Label>("MEM:--");
            memLabel->SetFontSize(font_size_);
            memLabel->SetTextColor(text_color_[0], text_color_[1], text_color_[2], text_color_[3]);
            container->AddChild(memLabel);
        }
        
        // Add Swap label
        if (showMemory && showSwap) {
            swapLabel = std::make_shared<Label>("");
            swapLabel->SetFontSize(font_size_);
            swapLabel->SetTextColor(text_color_[0], text_color_[1], text_color_[2], text_color_[3]);
            container->AddChild(swapLabel);
        }
        
        // Add Disk labels
        if (showDisks) {
            for (size_t i = 0; i < diskMounts.size(); i++) {
                auto diskLabel = std::make_shared<Label>("disk:--");
                diskLabel->SetFontSize(font_size_);
                diskLabel->SetTextColor(text_color_[0], text_color_[1], text_color_[2], text_color_[3]);
                diskLabels.push_back(diskLabel);
                container->AddChild(diskLabel);
            }
        }
        
        container->SetSpacing(10);
        
        // Initial CPU reading
        readCPUStats(prevCPUStats);
        
        return true;
    }
    
    void UpdateData() override {
        // Update CPU
        if (showCPU && cpuLabel) {
            CPUStats currStats;
            if (readCPUStats(currStats)) {
                cpuUsage = calculateCPUUsage(prevCPUStats, currStats);
                prevCPUStats = currStats;
                
                std::ostringstream oss;
                oss << "CPU:" << std::fixed << std::setprecision(1) << cpuUsage << "%";
                cpuLabel->SetText(oss.str());
            }
        }
        
        // Update Memory
        if (showMemory && memLabel) {
            if (readMemoryInfo()) {
                unsigned long long memUsed = memInfo.total - memInfo.available;
                double memPercent = (100.0 * memUsed) / memInfo.total;
                
                std::ostringstream oss;
                oss << "MEM:" << formatMemorySize(memUsed) 
                    << "/" << formatMemorySize(memInfo.total)
                    << "(" << static_cast<int>(memPercent) << "%)";
                memLabel->SetText(oss.str());
                
                // Update swap
                if (showSwap && swapLabel && memInfo.swapTotal > 0) {
                    unsigned long long swapUsed = memInfo.swapTotal - memInfo.swapFree;
                    if (swapUsed > 0) {
                        double swapPercent = (100.0 * swapUsed) / memInfo.swapTotal;
                        std::ostringstream swapOss;
                        swapOss << "SWAP:" << formatMemorySize(swapUsed)
                                << "(" << static_cast<int>(swapPercent) << "%)";
                        swapLabel->SetText(swapOss.str());
                    } else {
                        swapLabel->SetText("");
                    }
                }
            }
        }
        
        // Update Disks
        if (showDisks) {
            readDiskInfo();
            for (size_t i = 0; i < diskInfo.size() && i < diskLabels.size(); i++) {
                std::string mountName = diskInfo[i].mount;
                if (mountName == "/") {
                    mountName = "root";
                } else if (mountName[0] == '/') {
                    mountName = mountName.substr(1);
                }
                
                std::ostringstream oss;
                oss << mountName << ":" << diskInfo[i].used << "G/"
                    << diskInfo[i].total << "G(" 
                    << static_cast<int>(diskInfo[i].usedPercent) << "%)";
                diskLabels[i]->SetText(oss.str());
            }
        }
        
        MarkNeedsPaint();  // Flutter-style dirty tracking
    }
    
    void CalculateSize(int available_width, int available_height) override {
        // No lock needed - main thread only
        
        if (container) {
            container->CalculateSize(available_width, available_height);
            width_ = container->GetWidth();
            height_ = container->GetHeight();
        }
    }
    
    void Render(cairo_t* cr) override {
        if (!IsVisible() || !container) return;
        
        // No lock needed - main thread only
        
        container->SetPosition(x_, y_);
        container->Render(cr);
    }

private:
    struct CPUStats {
        unsigned long long user, nice, system, idle, iowait, irq, softirq;
    };
    
    struct MemoryInfo {
        unsigned long long total, available, swapTotal, swapFree;
    };
    
    struct DiskInfo {
        std::string mount;
        unsigned long long total, used;
        double usedPercent;
    };
    
    CPUStats prevCPUStats;
    double cpuUsage;
    MemoryInfo memInfo;
    std::vector<DiskInfo> diskInfo;
    
    bool showCPU, showMemory, showSwap, showDisks;
    std::vector<std::string> diskMounts;
    
    // UI components
    std::shared_ptr<HBox> container;
    std::shared_ptr<Label> cpuLabel;
    std::shared_ptr<Label> memLabel;
    std::shared_ptr<Label> swapLabel;
    std::vector<std::shared_ptr<Label>> diskLabels;
    
    bool readCPUStats(CPUStats& stats) {
        std::ifstream file("/proc/stat");
        if (!file.is_open()) return false;
        
        std::string line;
        std::getline(file, line);
        
        if (line.substr(0, 3) != "cpu") return false;
        
        std::istringstream iss(line);
        std::string cpu;
        iss >> cpu >> stats.user >> stats.nice >> stats.system >> stats.idle 
            >> stats.iowait >> stats.irq >> stats.softirq;
        
        return true;
    }
    
    double calculateCPUUsage(const CPUStats& prev, const CPUStats& curr) {
        unsigned long long prevIdle = prev.idle + prev.iowait;
        unsigned long long currIdle = curr.idle + curr.iowait;
        
        unsigned long long prevTotal = prev.user + prev.nice + prev.system + 
                                       prev.idle + prev.iowait + prev.irq + prev.softirq;
        unsigned long long currTotal = curr.user + curr.nice + curr.system + 
                                       curr.idle + curr.iowait + curr.irq + curr.softirq;
        
        unsigned long long totalDiff = currTotal - prevTotal;
        unsigned long long idleDiff = currIdle - prevIdle;
        
        if (totalDiff == 0) return 0.0;
        
        return (100.0 * (totalDiff - idleDiff)) / totalDiff;
    }
    
    bool readMemoryInfo() {
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()) return false;
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key;
            unsigned long long value;
            
            iss >> key >> value;
            
            if (key == "MemTotal:") {
                memInfo.total = value;
            } else if (key == "MemAvailable:") {
                memInfo.available = value;
            } else if (key == "SwapTotal:") {
                memInfo.swapTotal = value;
            } else if (key == "SwapFree:") {
                memInfo.swapFree = value;
            }
        }
        
        return true;
    }
    
    void readDiskInfo() {
        diskInfo.clear();
        
        for (const auto& mount : diskMounts) {
            struct statvfs stat;
            if (statvfs(mount.c_str(), &stat) == 0) {
                DiskInfo info;
                info.mount = mount;
                info.total = (stat.f_blocks * stat.f_frsize) / (1024 * 1024 * 1024); // GB
                unsigned long long available = (stat.f_bavail * stat.f_frsize) / (1024 * 1024 * 1024);
                info.used = info.total - available;
                info.usedPercent = info.total > 0 ? (100.0 * info.used) / info.total : 0.0;
                
                diskInfo.push_back(info);
            }
        }
    }
    
    std::string formatMemorySize(unsigned long long kb) const {
        std::ostringstream oss;
        
        if (kb >= 1024 * 1024) {
            oss << std::fixed << std::setprecision(1) << (kb / (1024.0 * 1024.0)) << "G";
        } else if (kb >= 1024) {
            oss << std::fixed << std::setprecision(1) << (kb / 1024.0) << "M";
        } else {
            oss << kb << "K";
        }
        
        return oss.str();
    }
};

} // namespace UI
} // namespace Leviathan

// Export plugin functions
extern "C" {
    EXPORT_PLUGIN_CREATE(SystemMonitorWidget)
    EXPORT_PLUGIN_DESTROY(SystemMonitorWidget)
    EXPORT_PLUGIN_METADATA(SystemMonitorWidget)
}
