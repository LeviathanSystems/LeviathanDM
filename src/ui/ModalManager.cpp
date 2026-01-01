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
    LOG_INFO_FMT("Registered modal type: '{}'", name);
}

void ModalManager::UnregisterModalType(const std::string& name) {
    GetModalRegistry().erase(name);
    LOG_INFO_FMT("Unregistered modal type: '{}'", name);
}

// Get a modal instance (creates it from factory)
std::unique_ptr<Modal> ModalManager::GetModal(const std::string& name) {
    auto& registry = GetModalRegistry();
    auto it = registry.find(name);
    if (it == registry.end()) {
        LOG_ERROR_FMT("Modal type '{}' not registered", name);
        return nullptr;
    }
    
    // Create modal using factory
    auto modal = it->second();
    if (!modal) {
        LOG_ERROR_FMT("Failed to create modal '{}'", name);
        return nullptr;
    }
    
    LOG_DEBUG_FMT("Created modal instance: '{}'", name);
    return modal;
}

} // namespace UI
} // namespace Leviathan
