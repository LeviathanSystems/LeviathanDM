#include "ui/menubar/providers/CommandsProvider.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <unistd.h>

namespace Leviathan {
namespace UI {

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

} // namespace UI
} // namespace Leviathan
