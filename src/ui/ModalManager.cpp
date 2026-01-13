#include "ui/ModalManager.hpp"
#include "ui/reusable-widgets/BaseModal.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace UI {

// Static modal registry accessor
std::unordered_map<std::string, ModalFactory>& ModalManager::GetModalRegistry() {
    static std::unordered_map<std::string, ModalFactory> registry;
    return registry;
}

// Static registration methods
void ModalManager::RegisterModalType(const std::string& name, ModalFactory factory) {
    GetModalRegistry()[name] = factory;
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Registered modal type: '{}'", name);
}

void ModalManager::UnregisterModalType(const std::string& name) {
    GetModalRegistry().erase(name);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Unregistered modal type: '{}'", name);
}

// Get a modal instance (creates it from factory)
std::unique_ptr<Modal> ModalManager::GetModal(const std::string& name) {
    auto& registry = GetModalRegistry();
    auto it = registry.find(name);
    if (it == registry.end()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Modal type '{}' not registered", name);
        return nullptr;
    }
    
    // Create modal using factory
    auto modal = it->second();
    if (!modal) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to create modal '{}'", name);
        return nullptr;
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Created modal instance: '{}'", name);
    return modal;
}

} // namespace UI
} // namespace Leviathan
