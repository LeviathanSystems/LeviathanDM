#include "KeyBindings.hpp"
#include "wayland/Server.hpp"
#include "ui/menubar/MenuBarManager.hpp"
#include "Logger.hpp"

namespace Leviathan {

using Wayland::Server;

// Static instance
KeyBindings* KeyBindings::instance_ = nullptr;

KeyBindings::KeyBindings(Server* server)
    : server_(server)
    , action_registry_(std::make_unique<ActionRegistry>(server)) {
    
    // Register custom spawn actions with specific commands
    action_registry_->RegisterAction({
        .name = "open-terminal",
        .description = "Open a new terminal window",
        .type = ActionType::SPAWN,
        .params = {{"command", "alacritty"}}
    });
    
    action_registry_->RegisterAction({
        .name = "open-browser",
        .description = "Open web browser",
        .type = ActionType::SPAWN,
        .params = {{"command", "firefox"}}
    });
    
    // Register tag switch actions
    for (int i = 0; i < 9; ++i) {
        action_registry_->RegisterAction({
            .name = "switch-to-tag-" + std::to_string(i + 1),
            .description = "Switch to tag " + std::to_string(i + 1),
            .type = ActionType::SWITCH_TO_TAG,
            .params = {{"tag_index", std::to_string(i)}}
        });
        
        action_registry_->RegisterAction({
            .name = "move-to-tag-" + std::to_string(i + 1),
            .description = "Move focused window to tag " + std::to_string(i + 1),
            .type = ActionType::MOVE_TO_TAG,
            .params = {{"tag_index", std::to_string(i)}}
        });
    }
    
    SetupDefaultBindings();
}

void KeyBindings::SetupDefaultBindings() {
    // Mod key is Super (Windows key)
    uint32_t mod = MOD_SUPER;
    
    // Window management
    AddBinding(mod, XKB_KEY_Return, "open-terminal");
    AddBinding(mod | MOD_SHIFT, XKB_KEY_C, "close-window");
    AddBinding(mod | MOD_SHIFT, XKB_KEY_Q, "shutdown");
    
    // Focus navigation
    AddBinding(mod, XKB_KEY_j, "focus-next");
    AddBinding(mod, XKB_KEY_k, "focus-prev");
    
    // Window swapping
    AddBinding(mod | MOD_SHIFT, XKB_KEY_J, "swap-next");
    AddBinding(mod | MOD_SHIFT, XKB_KEY_K, "swap-prev");
    
    // Floating and fullscreen
    AddBinding(mod, XKB_KEY_f, "toggle-floating");
    AddBinding(mod | MOD_SHIFT, XKB_KEY_F, "toggle-fullscreen");
    
    // Tag switching (1-9)
    for (int i = 1; i <= 9; ++i) {
        xkb_keysym_t key = XKB_KEY_1 + (i - 1);
        AddBinding(mod | MOD_SHIFT | MOD_CTRL, key, "switch-to-tag-" + std::to_string(i));
        //AddBinding(mod | MOD_SHIFT, key, "move-to-tag-" + std::to_string(i));
    }
    
    // Application launcher / menubar
    AddBinding(mod, XKB_KEY_p, "toggle-menubar");
    
    // Help
    AddBinding(mod, XKB_KEY_F1, "show-help");
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Loaded {} key bindings", bindings_.size());
}

void KeyBindings::AddBinding(uint32_t modifiers, xkb_keysym_t keysym, const std::string& action_name) {
    bindings_.push_back({modifiers, keysym, action_name});
}

bool KeyBindings::HandleKeyPress(uint32_t modifiers, xkb_keysym_t keysym) {
    for (const auto& binding : bindings_) {
        if (binding.keysym == keysym && binding.modifiers == modifiers) {
            // Log which keybinding is being triggered
            std::string mod_str;
            if (modifiers & MOD_SUPER) mod_str += "Super+";
            if (modifiers & MOD_SHIFT) mod_str += "Shift+";
            if (modifiers & MOD_CTRL)  mod_str += "Ctrl+";
            if (modifiers & MOD_ALT)   mod_str += "Alt+";
            
            // Get key name from keysym
            char key_name[64];
            xkb_keysym_get_name(keysym, key_name, sizeof(key_name));
            
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Keybinding triggered: {}{} -> {}", mod_str, key_name, binding.action_name);
            
            // Execute the action
            if (action_registry_->ExecuteAction(binding.action_name)) {
                return true;
            } else {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to execute action: {}", binding.action_name);
                return false;
            }
        }
    }
    
    return false;
}

} // namespace Leviathan
