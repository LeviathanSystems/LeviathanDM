#pragma once

#include "ui/menubar/MenuBar.hpp"
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
    std::string GetTabName() const override { return "Apps"; }
    std::vector<std::shared_ptr<MenuItem>> LoadItems() override;
    
private:
    void LoadDesktopFilesFromDirectory(const std::string& dir,
                                      std::vector<std::shared_ptr<MenuItem>>& items,
                                      std::unordered_set<std::string>& seen_apps);
    std::shared_ptr<MenuItem> ParseDesktopFile(const std::string& filepath);
    
    std::vector<std::string> search_paths_;
};

} // namespace UI
} // namespace Leviathan
