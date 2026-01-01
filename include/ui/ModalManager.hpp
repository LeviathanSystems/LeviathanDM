#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace Leviathan {
namespace UI {

// Forward declaration
class Modal;

// Modal factory function type
using ModalFactory = std::function<std::unique_ptr<Modal>()>;

/**
 * @brief Simple modal registry
 * 
 * ModalManager is just a static registry that maps modal names to factory functions.
 * It does NOT handle:
 * - Opening/closing modals (LayerManager does this)
 * - Rendering (LayerManager does this)
 * - Output dimensions (LayerManager handles this)
 * 
 * It ONLY handles:
 * - Static modal type registration at startup
 * - Getting registered modal instances
 */
class ModalManager {
public:
    // Static modal registration (called at startup)
    static void RegisterModalType(const std::string& name, ModalFactory factory);
    static void UnregisterModalType(const std::string& name);
    
    // Get a modal instance by name (creates it if factory exists)
    static std::unique_ptr<Modal> GetModal(const std::string& name);
    
private:
    // Static modal type registry
    static std::unordered_map<std::string, ModalFactory>& GetModalRegistry();
};

} // namespace UI
} // namespace Leviathan
