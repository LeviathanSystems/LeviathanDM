#include "wayland/Input.hpp"
#include "wayland/Server.hpp"
#include "Logger.hpp"

extern "C" {
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/backend/multi.h>
#include <wlr/backend/libinput.h>
#include <libinput.h>
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
    
    // Get keysyms first
    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t* syms;
    int nsyms = xkb_state_key_get_syms(
        keyboard->wlr_keyboard->xkb_state, keycode, &syms);
    
    // Get modifiers - this reflects the state INCLUDING the current key press
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
    bool ctrl = modifiers & WLR_MODIFIER_CTRL;
    bool alt = modifiers & WLR_MODIFIER_ALT;
    bool shift = modifiers & WLR_MODIFIER_SHIFT;
    bool mod = modifiers & WLR_MODIFIER_LOGO;
    
    // CRITICAL: Allow VT switching (Ctrl+Alt+F1-F12) to pass through to backend
    // Don't consume these keys - the libseat/session backend handles VT switching
    if (ctrl && alt) {
        for (int i = 0; i < nsyms; i++) {
            // Check for F1-F12 keys (XKB_KEY_F1 = 0xffbe through XKB_KEY_F12 = 0xffc9)
            if (syms[i] >= XKB_KEY_F1 && syms[i] <= XKB_KEY_F12) {
                // This is a VT switch key - don't consume it, backend will handle
                return;
            }
        }
    }
    
    bool handled = false;
    
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            xkb_keysym_t sym = syms[i];
            
            // Skip modifier keys themselves - we only care about regular keys with modifiers
            if (sym == XKB_KEY_Shift_L || sym == XKB_KEY_Shift_R ||
                sym == XKB_KEY_Control_L || sym == XKB_KEY_Control_R ||
                sym == XKB_KEY_Alt_L || sym == XKB_KEY_Alt_R ||
                sym == XKB_KEY_Super_L || sym == XKB_KEY_Super_R ||
                sym == XKB_KEY_Meta_L || sym == XKB_KEY_Meta_R) {
                continue; // Don't process modifier keys themselves
            }
            
            // Debug: Log key presses with modifiers (for non-modifier keys only)
            LOG_DEBUG("Key pressed: sym={:#x}, mod={}, shift={}, ctrl={}, alt={}", 
                     sym, mod, shift, ctrl, alt);
            
            // Mod+Shift+Q: Quit compositor
            // Check if it's actually Q key (not a modifier)
            if (sym == XKB_KEY_Q || sym == XKB_KEY_q) {
                if (mod && shift) {
                    LOG_INFO("Mod+Shift+Q pressed - Terminating compositor");
                    wl_display_terminate(server->GetDisplay());
                    handled = true;
                    break;
                }
            }
            
            // Future: Add more keybindings here
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
        
        // Configure pointer acceleration/speed if device supports libinput
        if (wlr_input_device_is_libinput(device)) {
            struct libinput_device* libinput_dev = wlr_libinput_get_device_handle(device);
            
            // Get mouse speed from config (default: 0.5)
            double mouse_speed = 0.5;
            if (server->GetConfigParser()) {
                mouse_speed = server->GetConfigParser()->libinput.mouse.speed;
            }
            
            // Set pointer acceleration (range: -1 to 1)
            if (libinput_device_config_accel_is_available(libinput_dev)) {
                libinput_device_config_accel_set_speed(libinput_dev, mouse_speed);
                LOG_INFO("Set pointer speed to {} for device: {}", mouse_speed, device->name);
            }
            
            // Enable pointer acceleration profile
            if (libinput_device_config_accel_get_profiles(libinput_dev) & 
                LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE) {
                libinput_device_config_accel_set_profile(libinput_dev, 
                    LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE);
            }
        }
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
    Server* server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event* event = 
        static_cast<struct wlr_pointer_motion_event*>(data);
    
    // Move cursor by relative delta
    wlr_cursor_move(server->cursor, &event->pointer->base,
                    event->delta_x, event->delta_y);
    
    // TODO: Process cursor motion (find surface under cursor, send pointer events)
}

void InputManager::HandleCursorMotionAbsolute(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event* event = 
        static_cast<struct wlr_pointer_motion_absolute_event*>(data);
    
    // Warp cursor to absolute position (0..1 coordinates)
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, 
                            event->x, event->y);
    
    // TODO: Process cursor motion
}

void InputManager::HandleCursorButton(struct wl_listener* listener, void* data) {
    // Future: handle mouse buttons
}

void InputManager::HandleCursorAxis(struct wl_listener* listener, void* data) {
    // Future: handle scroll
}

void InputManager::HandleCursorFrame(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, cursor_frame);
    // Notify seat of pointer frame (groups events together)
    wlr_seat_pointer_notify_frame(server->seat);
}

void InputManager::HandleRequestCursor(struct wl_listener* listener, void* data) {
    // Future: handle cursor requests
}

void InputManager::HandleRequestSetSelection(struct wl_listener* listener, void* data) {
    // Future: handle clipboard
}

} // namespace Wayland
} // namespace Leviathan
