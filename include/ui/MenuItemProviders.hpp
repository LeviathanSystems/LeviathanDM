#pragma once

#include "ui/MenuBar.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

namespace Leviathan {
namespace UI {

/**
 * @brief Desktop Application menu item
 * 
 * Represents a .desktop file application
 */
class DesktopAppMenuItem : public MenuItem {
public:
    DesktopAppMenuItem(const std::string& name,
                       const std::string& exec,
                       const std::string& icon = "",
                       const std::string& description = "",
                       const std::vector<std::string>& categories = {});
    
    std::string GetDisplayName() const override { return name_; }
    std::vector<std::string> GetSearchKeywords() const override;
    std::string GetIconPath() const override { return icon_; }
    std::string GetDescription() const override { return description_; }
    void Execute() override;
    
    // For deduplication
    std::string GetExecCommand() const { return exec_; }
    
private:
    std::string name_;
    std::string exec_;
    std::string icon_;
    std::string description_;
    std::vector<std::string> categories_;
};

/**
 * @brief Provider that loads desktop applications from .desktop files
 */
class DesktopApplicationProvider : public IMenuItemProvider {
public:
    DesktopApplicationProvider();
    
    std::string GetName() const override { return "Desktop Applications"; }
    std::vector<std::shared_ptr<MenuItem>> LoadItems() override;
    
private:
    void LoadDesktopFilesFromDirectory(const std::string& dir,
                                      std::vector<std::shared_ptr<MenuItem>>& items,
                                      std::unordered_set<std::string>& seen_apps);
    std::shared_ptr<MenuItem> ParseDesktopFile(const std::string& filepath);
    
    std::vector<std::string> search_paths_;
};

/**
 * @brief Custom command menu item
 * 
 * Execute a custom shell command or function
 */
class CustomCommandMenuItem : public MenuItem {
public:
    using CommandFunc = std::function<void()>;
    
    CustomCommandMenuItem(const std::string& name,
                          const std::string& command,
                          const std::string& description = "",
                          int priority = 0);
    
    CustomCommandMenuItem(const std::string& name,
                          CommandFunc func,
                          const std::string& description = "",
                          int priority = 0);
    
    std::string GetDisplayName() const override { return name_; }
    std::vector<std::string> GetSearchKeywords() const override;
    std::string GetDescription() const override { return description_; }
    void Execute() override;
    int GetPriority() const override { return priority_; }
    
private:
    std::string name_;
    std::string command_;  // Shell command to execute
    CommandFunc func_;     // Or function to call
    std::string description_;
    int priority_;
    bool use_function_;
};

/**
 * @brief Provider for custom commands
 * 
 * Allows adding custom commands, scripts, or functions to the menu
 */
class CustomCommandProvider : public IMenuItemProvider {
public:
    CustomCommandProvider();
    
    std::string GetName() const override { return "Custom Commands"; }
    std::vector<std::shared_ptr<MenuItem>> LoadItems() override;
    bool SupportsLiveUpdates() const override { return true; }
    void Refresh() override;
    
    // Add custom commands
    void AddCommand(const std::string& name,
                   const std::string& command,
                   const std::string& description = "",
                   int priority = 0);
    
    void AddCommand(const std::string& name,
                   std::function<void()> func,
                   const std::string& description = "",
                   int priority = 0);
    
    void ClearCommands();
    
private:
    std::vector<std::shared_ptr<MenuItem>> commands_;
};

/**
 * @brief Bookmark menu item
 * 
 * Quick links to directories, files, or URLs
 */
class BookmarkMenuItem : public MenuItem {
public:
    enum class BookmarkType {
        Directory,
        File,
        URL
    };
    
    BookmarkMenuItem(const std::string& name,
                    const std::string& target,
                    BookmarkType type,
                    const std::string& description = "");
    
    std::string GetDisplayName() const override { return name_; }
    std::vector<std::string> GetSearchKeywords() const override;
    std::string GetDescription() const override { return description_; }
    void Execute() override;
    
private:
    std::string name_;
    std::string target_;
    BookmarkType type_;
    std::string description_;
};

/**
 * @brief Provider for bookmarks
 */
class BookmarkProvider : public IMenuItemProvider {
public:
    BookmarkProvider();
    
    std::string GetName() const override { return "Bookmarks"; }
    std::vector<std::shared_ptr<MenuItem>> LoadItems() override;
    bool SupportsLiveUpdates() const override { return true; }
    void Refresh() override;
    
    void AddBookmark(const std::string& name,
                    const std::string& target,
                    BookmarkMenuItem::BookmarkType type,
                    const std::string& description = "");
    
    void ClearBookmarks();
    
private:
    std::vector<std::shared_ptr<MenuItem>> bookmarks_;
};

} // namespace UI
} // namespace Leviathan
