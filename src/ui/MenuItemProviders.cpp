#include "ui/MenuItemProviders.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
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
    
    for (const auto& path : search_paths_) {
        LoadDesktopFilesFromDirectory(path, items);
    }
    
    return items;
}

void DesktopApplicationProvider::LoadDesktopFilesFromDirectory(
    const std::string& dir,
    std::vector<std::shared_ptr<MenuItem>>& items)
{
    try {
        if (!std::filesystem::exists(dir)) {
            return;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
                auto item = ParseDesktopFile(entry.path().string());
                if (item) {
                    items.push_back(item);
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

// ============================================================================
// CustomCommandMenuItem
// ============================================================================

CustomCommandMenuItem::CustomCommandMenuItem(const std::string& name,
                                             const std::string& command,
                                             const std::string& description,
                                             int priority)
    : name_(name)
    , command_(command)
    , description_(description)
    , priority_(priority)
    , use_function_(false)
{
}

CustomCommandMenuItem::CustomCommandMenuItem(const std::string& name,
                                             CommandFunc func,
                                             const std::string& description,
                                             int priority)
    : name_(name)
    , func_(func)
    , description_(description)
    , priority_(priority)
    , use_function_(true)
{
}

std::vector<std::string> CustomCommandMenuItem::GetSearchKeywords() const {
    return {name_, description_};
}

void CustomCommandMenuItem::Execute() {
    if (use_function_) {
        if (func_) {
            func_();
        }
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            execl("/bin/sh", "sh", "-c", command_.c_str(), nullptr);
            exit(1);
        } else if (pid < 0) {
            LOG_ERROR_FMT("Failed to fork process for command: {}", name_);
        } else {
            LOG_INFO_FMT("Executed command: {} (PID: {})", name_, pid);
        }
    }
}

// ============================================================================
// CustomCommandProvider
// ============================================================================

CustomCommandProvider::CustomCommandProvider() {
}

std::vector<std::shared_ptr<MenuItem>> CustomCommandProvider::LoadItems() {
    return commands_;
}

void CustomCommandProvider::Refresh() {
    // Commands are managed directly, no refresh needed
}

void CustomCommandProvider::AddCommand(const std::string& name,
                                       const std::string& command,
                                       const std::string& description,
                                       int priority)
{
    commands_.push_back(std::make_shared<CustomCommandMenuItem>(
        name, command, description, priority));
}

void CustomCommandProvider::AddCommand(const std::string& name,
                                       std::function<void()> func,
                                       const std::string& description,
                                       int priority)
{
    commands_.push_back(std::make_shared<CustomCommandMenuItem>(
        name, func, description, priority));
}

void CustomCommandProvider::ClearCommands() {
    commands_.clear();
}

// ============================================================================
// BookmarkMenuItem
// ============================================================================

BookmarkMenuItem::BookmarkMenuItem(const std::string& name,
                                   const std::string& target,
                                   BookmarkType type,
                                   const std::string& description)
    : name_(name)
    , target_(target)
    , type_(type)
    , description_(description)
{
}

std::vector<std::string> BookmarkMenuItem::GetSearchKeywords() const {
    return {name_, target_, description_};
}

void BookmarkMenuItem::Execute() {
    std::string command;
    
    switch (type_) {
        case BookmarkType::Directory:
            // Open file manager in directory
            command = "xdg-open \"" + target_ + "\"";
            break;
        case BookmarkType::File:
            // Open file with default application
            command = "xdg-open \"" + target_ + "\"";
            break;
        case BookmarkType::URL:
            // Open URL in browser
            command = "xdg-open \"" + target_ + "\"";
            break;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        exit(1);
    } else if (pid < 0) {
        LOG_ERROR_FMT("Failed to open bookmark: {}", name_);
    } else {
        LOG_INFO_FMT("Opened bookmark: {}", name_);
    }
}

// ============================================================================
// BookmarkProvider
// ============================================================================

BookmarkProvider::BookmarkProvider() {
}

std::vector<std::shared_ptr<MenuItem>> BookmarkProvider::LoadItems() {
    return bookmarks_;
}

void BookmarkProvider::Refresh() {
    // Bookmarks are managed directly, no refresh needed
}

void BookmarkProvider::AddBookmark(const std::string& name,
                                  const std::string& target,
                                  BookmarkMenuItem::BookmarkType type,
                                  const std::string& description)
{
    bookmarks_.push_back(std::make_shared<BookmarkMenuItem>(
        name, target, type, description));
}

void BookmarkProvider::ClearBookmarks() {
    bookmarks_.clear();
}

} // namespace UI
} // namespace Leviathan
