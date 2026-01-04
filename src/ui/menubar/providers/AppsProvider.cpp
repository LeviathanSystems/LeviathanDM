#include "ui/menubar/providers/AppsProvider.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <filesystem>

namespace Leviathan {
namespace UI {

// ============================================================================
// DesktopAppMenuItem
// ============================================================================

DesktopAppMenuItem::DesktopAppMenuItem(const std::string& name,
                                       const std::string& exec,
                                       const std::string& icon,
                                       const std::string& description,
                                       const std::vector<std::string>& categories)
    : name_(name)
    , exec_(exec)
    , icon_(icon)
    , description_(description)
    , categories_(categories)
{
}

std::vector<std::string> DesktopAppMenuItem::GetSearchKeywords() const {
    std::vector<std::string> keywords = categories_;
    keywords.push_back(name_);
    if (!description_.empty()) {
        keywords.push_back(description_);
    }
    return keywords;
}

void DesktopAppMenuItem::Execute() {
    // Remove field codes like %f, %F, %u, %U from exec
    std::string cleaned_exec = exec_;
    size_t pos = 0;
    while ((pos = cleaned_exec.find('%', pos)) != std::string::npos) {
        if (pos + 1 < cleaned_exec.length()) {
            cleaned_exec.erase(pos, 2);
        } else {
            break;
        }
    }
    
    // Fork and execute
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        setsid();  // Create new session
        execl("/bin/sh", "sh", "-c", cleaned_exec.c_str(), nullptr);
        exit(1);  // If execl fails
    } else if (pid < 0) {
        LOG_ERROR_FMT("Failed to fork process for: {}", name_);
    } else {
        LOG_INFO_FMT("Launched application: {} (PID: {})", name_, pid);
    }
}

// ============================================================================
// DesktopApplicationProvider
// ============================================================================

DesktopApplicationProvider::DesktopApplicationProvider() {
    // Standard XDG paths
    search_paths_.push_back("/usr/share/applications");
    search_paths_.push_back("/usr/local/share/applications");
    
    // User local applications
    const char* home = getenv("HOME");
    if (home) {
        search_paths_.push_back(std::string(home) + "/.local/share/applications");
    }
    
    // Check XDG_DATA_DIRS
    const char* xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs) {
        std::stringstream ss(xdg_data_dirs);
        std::string path;
        while (std::getline(ss, path, ':')) {
            if (!path.empty()) {
                search_paths_.push_back(path + "/applications");
            }
        }
    }
}

std::vector<std::shared_ptr<MenuItem>> DesktopApplicationProvider::LoadItems() {
    std::vector<std::shared_ptr<MenuItem>> items;
    std::unordered_set<std::string> seen_apps;  // Track unique apps by name+exec
    
    for (const auto& path : search_paths_) {
        LoadDesktopFilesFromDirectory(path, items, seen_apps);
    }
    
    return items;
}

void DesktopApplicationProvider::LoadDesktopFilesFromDirectory(
    const std::string& dir,
    std::vector<std::shared_ptr<MenuItem>>& items,
    std::unordered_set<std::string>& seen_apps)
{
    try {
        if (!std::filesystem::exists(dir)) {
            return;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
                auto item = ParseDesktopFile(entry.path().string());
                if (item) {
                    // Create unique key from name + exec to detect duplicates
                    std::string unique_key = item->GetDisplayName() + "|" + 
                                           dynamic_cast<DesktopAppMenuItem*>(item.get())->GetExecCommand();
                    
                    // Only add if not seen before
                    if (seen_apps.find(unique_key) == seen_apps.end()) {
                        seen_apps.insert(unique_key);
                        items.push_back(item);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Failed to read directory {}: {}", dir, e.what());
    }
}

std::shared_ptr<MenuItem> DesktopApplicationProvider::ParseDesktopFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return nullptr;
    }
    
    std::string name, exec, icon, description;
    std::vector<std::string> categories;
    bool in_desktop_entry = false;
    bool no_display = false;
    bool hidden = false;
    
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') continue;
        
        if (line == "[Desktop Entry]") {
            in_desktop_entry = true;
            continue;
        } else if (line[0] == '[') {
            in_desktop_entry = false;
            continue;
        }
        
        if (!in_desktop_entry) continue;
        
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        if (key == "Name") {
            name = value;
        } else if (key == "Exec") {
            exec = value;
        } else if (key == "Icon") {
            icon = value;
        } else if (key == "Comment") {
            description = value;
        } else if (key == "Categories") {
            std::stringstream ss(value);
            std::string cat;
            while (std::getline(ss, cat, ';')) {
                if (!cat.empty()) {
                    categories.push_back(cat);
                }
            }
        } else if (key == "NoDisplay" && value == "true") {
            no_display = true;
        } else if (key == "Hidden" && value == "true") {
            hidden = true;
        }
    }
    
    // Skip if no display or hidden
    if (no_display || hidden || name.empty() || exec.empty()) {
        return nullptr;
    }
    
    return std::make_shared<DesktopAppMenuItem>(name, exec, icon, description, categories);
}

} // namespace UI
} // namespace Leviathan
