#ifndef KEY_BINDINGS_HPP
#define KEY_BINDINGS_HPP

#include "Actions.hpp"
#include <xkbcommon/xkbcommon.h>
#include <string>
#include <vector>
#include <memory>

namespace Leviathan {

namespace Wayland {
    class Server;
}

struct KeyBinding {
    uint32_t modifiers;
    xkb_keysym_t keysym;
    std::string action_name;  // Name of action to execute
};

class KeyBindings {
public:
    KeyBindings(Wayland::Server* server);
    
    // Singleton access
    static KeyBindings* Instance() { return instance_; }
    static void SetInstance(KeyBindings* instance) { instance_ = instance; }
    
    bool HandleKeyPress(uint32_t modifiers, xkb_keysym_t keysym);
    
    // Add a key binding programmatically
    void AddBinding(uint32_t modifiers, xkb_keysym_t keysym, const std::string& action_name);
    
    // Get action registry (for adding custom actions)
    ActionRegistry* GetActionRegistry() { return action_registry_.get(); }
    
    // Get all bindings (for help display)
    const std::vector<KeyBinding>& GetBindings() const { return bindings_; }
    
private:
    void SetupDefaultBindings();
    
private:
    static KeyBindings* instance_;
    Wayland::Server* server_;
    std::unique_ptr<ActionRegistry> action_registry_;
    std::vector<KeyBinding> bindings_;
};

} // namespace Leviathan

#endif // KEY_BINDINGS_HPP
