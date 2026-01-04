#pragma once

#include "ui/menubar/MenuBar.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Leviathan {
namespace UI {

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
    std::string GetTabName() const override { return "Commands"; }
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

} // namespace UI
} // namespace Leviathan
