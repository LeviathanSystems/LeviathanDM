#ifndef INPUT_HPP
#define INPUT_HPP

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <xkbcommon/xkbcommon.h>
}

namespace Leviathan {
namespace Wayland {

class Server;

struct Keyboard {
    struct wlr_keyboard* wlr_keyboard;
    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;
    struct wl_list link;
    
    Server* server;
};

class InputManager {
public:
    static void HandleNewInput(Server* server, struct wlr_input_device* device);
    static void HandleKeyboardKey(struct wl_listener* listener, void* data);
    static void HandleKeyboardModifiers(struct wl_listener* listener, void* data);
    static void HandleKeyboardDestroy(struct wl_listener* listener, void* data);
    
    static void HandleCursorMotion(struct wl_listener* listener, void* data);
    static void HandleCursorMotionAbsolute(struct wl_listener* listener, void* data);
    static void HandleCursorButton(struct wl_listener* listener, void* data);
    static void HandleCursorAxis(struct wl_listener* listener, void* data);
    static void HandleCursorFrame(struct wl_listener* listener, void* data);
    
    static void HandleRequestCursor(struct wl_listener* listener, void* data);
    static void HandleRequestSetSelection(struct wl_listener* listener, void* data);
};

} // namespace Wayland
} // namespace Leviathan

#endif // INPUT_HPP
