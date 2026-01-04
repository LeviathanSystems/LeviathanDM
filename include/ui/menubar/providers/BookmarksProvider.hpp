#pragma once

#include "ui/menubar/MenuBar.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Leviathan {
namespace UI {

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
    std::string GetTabName() const override { return "Bookmarks"; }
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
