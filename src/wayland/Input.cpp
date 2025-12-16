#include "wayland/Input.hpp"
#include "wayland/Server.hpp"

extern "C" {
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/backend/multi.h>
#include <stdlib.h>
}

namespace Leviathan {
namespace Wayland {

static void keyboard_handle_modifiers(struct wl_listener* listener, void* data) {
    Keyboard* keyboard = wl_container_of(listener, keyboard, modifiers);
    wlr_seat_set_keyboard(keyboard->server->GetSeat(), keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->GetSeat(),
        &keyboard->wlr_keyboard->modifiers);
}

static void keyboard_handle_key(struct wl_listener* listener, void* data) {
    Keyboard* keyboard = wl_container_of(listener, keyboard, key);
    Server* server = keyboard->server;
    struct wlr_keyboard_key_event* event = static_cast<struct wlr_keyboard_key_event*>(data);
    struct wlr_seat* seat = server->GetSeat();
    
    // Get keysyms
    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t* syms;
    int nsyms = xkb_state_key_get_syms(
        keyboard->wlr_keyboard->xkb_state, keycode, &syms);
    
    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
    
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            // Handle keybinding via KeyBindings class
            // Note: This is simplified - in production you'd store KeyBindings in Server
            handled = false; // Keybindings will be checked elsewhere
        }
    }
    
    if (!handled) {
        // Pass through to client
        wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec,
            event->keycode, event->state);
    }
}

static void keyboard_handle_destroy(struct wl_listener* listener, void* data) {
    Keyboard* keyboard = wl_container_of(listener, keyboard, destroy);
    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);
    delete keyboard;
}

void InputManager::HandleNewInput(Server* server, struct wlr_input_device* device) {
    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
        Keyboard* keyboard = new Keyboard();
        keyboard->wlr_keyboard = wlr_keyboard_from_input_device(device);
        keyboard->server = server;
        
        // Set keyboard layout
        struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        struct xkb_keymap* keymap = xkb_keymap_new_from_names(context, nullptr,
            XKB_KEYMAP_COMPILE_NO_FLAGS);
        
        wlr_keyboard_set_keymap(keyboard->wlr_keyboard, keymap);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        
        wlr_keyboard_set_repeat_info(keyboard->wlr_keyboard, 25, 600);
        
        // Setup listeners
        keyboard->modifiers.notify = keyboard_handle_modifiers;
        wl_signal_add(&keyboard->wlr_keyboard->events.modifiers, &keyboard->modifiers);
        
        keyboard->key.notify = keyboard_handle_key;
        wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);
        
        keyboard->destroy.notify = keyboard_handle_destroy;
        wl_signal_add(&device->events.destroy, &keyboard->destroy);
        
        wl_list_insert(&server->keyboards, &keyboard->link);
        
        wlr_seat_set_keyboard(server->GetSeat(), keyboard->wlr_keyboard);
        break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
        // Attach pointer to cursor
        wlr_cursor_attach_input_device(server->cursor, device);
        break;
    }
    default:
        break;
    }
    
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->GetSeat(), caps);
}

void InputManager::HandleKeyboardKey(struct wl_listener* listener, void* data) {
    keyboard_handle_key(listener, data);
}

void InputManager::HandleKeyboardModifiers(struct wl_listener* listener, void* data) {
    keyboard_handle_modifiers(listener, data);
}

void InputManager::HandleKeyboardDestroy(struct wl_listener* listener, void* data) {
    keyboard_handle_destroy(listener, data);
}

void InputManager::HandleCursorMotion(struct wl_listener* listener, void* data) {
    // Future: handle cursor motion
}

void InputManager::HandleCursorButton(struct wl_listener* listener, void* data) {
    // Future: handle mouse buttons
}

void InputManager::HandleCursorAxis(struct wl_listener* listener, void* data) {
    // Future: handle scroll
}

void InputManager::HandleCursorFrame(struct wl_listener* listener, void* data) {
    // Future: cursor frame
}

void InputManager::HandleRequestCursor(struct wl_listener* listener, void* data) {
    // Future: handle cursor requests
}

void InputManager::HandleRequestSetSelection(struct wl_listener* listener, void* data) {
    // Future: handle clipboard
}

} // namespace Wayland
} // namespace Leviathan
