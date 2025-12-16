#ifndef KEY_BINDINGS_HPP
#define KEY_BINDINGS_HPP

#include <xkbcommon/xkbcommon.h>
#include <functional>
#include <vector>

namespace Leviathan {

namespace Wayland {
    class Server;
}

struct KeyBinding {
    uint32_t modifiers;
    xkb_keysym_t keysym;
    std::function<void()> action;
};

class KeyBindings {
public:
    KeyBindings(Wayland::Server* server);
    
    bool HandleKeyPress(uint32_t modifiers, xkb_keysym_t keysym);
    
private:
    void SetupDefaultBindings();
    
private:
    Wayland::Server* server_;
    std::vector<KeyBinding> bindings_;
};

} // namespace Leviathan

#endif // KEY_BINDINGS_HPP
