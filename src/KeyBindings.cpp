#include "KeyBindings.hpp"
#include "wayland/Server.hpp"

#include <iostream>
#include <cstdlib>

namespace Leviathan {

using Wayland::Server;

KeyBindings::KeyBindings(Server* server)
    : server_(server) {
    SetupDefaultBindings();
}

void KeyBindings::SetupDefaultBindings() {
    // Mod key is Super (Windows key)
    uint32_t mod = MOD_SUPER;
    
    // Window management
    bindings_.push_back({mod, XKB_KEY_Return, [this]() {
        // Launch terminal
        system("alacritty &");
    }});
    
    bindings_.push_back({mod | MOD_SHIFT, XKB_KEY_c, [this]() {
        server_->CloseView(nullptr);
    }});
    
    bindings_.push_back({mod | MOD_SHIFT, XKB_KEY_q, [this]() {
        exit(0);
    }});
    
    // Focus navigation
    bindings_.push_back({mod, XKB_KEY_j, [this]() {
        server_->FocusNext();
    }});
    
    bindings_.push_back({mod, XKB_KEY_k, [this]() {
        server_->FocusPrev();
    }});
    
    // Window swapping
    bindings_.push_back({mod | MOD_SHIFT, XKB_KEY_j, [this]() {
        server_->SwapWithNext();
    }});
    
    bindings_.push_back({mod | MOD_SHIFT, XKB_KEY_k, [this]() {
        server_->SwapWithPrev();
    }});
    
    // Layout control
    bindings_.push_back({mod, XKB_KEY_h, [this]() {
        server_->DecreaseMasterRatio();
    }});
    
    bindings_.push_back({mod, XKB_KEY_l, [this]() {
        server_->IncreaseMasterRatio();
    }});
    
    bindings_.push_back({mod, XKB_KEY_i, [this]() {
        server_->IncreaseMasterCount();
    }});
    
    bindings_.push_back({mod, XKB_KEY_d, [this]() {
        server_->DecreaseMasterCount();
    }});
    
    // Layout switching
    bindings_.push_back({mod, XKB_KEY_t, [this]() {
        server_->SetLayout(LayoutType::MASTER_STACK);
    }});
    
    bindings_.push_back({mod, XKB_KEY_m, [this]() {
        server_->SetLayout(LayoutType::MONOCLE);
    }});
    
    bindings_.push_back({mod, XKB_KEY_g, [this]() {
        server_->SetLayout(LayoutType::GRID);
    }});
    
    // Workspace switching (1-9)
    for (int i = 1; i <= 9; ++i) {
        xkb_keysym_t key = XKB_KEY_1 + (i - 1);
        bindings_.push_back({mod, key, [this, i]() {
            server_->SwitchToTag(i - 1);
        }});
        
        // Move window to tag
        bindings_.push_back({mod | MOD_SHIFT, key, [this, i]() {
            server_->MoveClientToTag(i - 1);
        }});
    }
    
    // Application launcher
    bindings_.push_back({mod, XKB_KEY_p, [this]() {
        system("rofi -show run &");
    }});
}

bool KeyBindings::HandleKeyPress(uint32_t modifiers, xkb_keysym_t keysym) {
    for (const auto& binding : bindings_) {
        if (binding.keysym == keysym && binding.modifiers == modifiers) {
            binding.action();
            return true;
        }
    }
    return false;
}

} // namespace Leviathan
