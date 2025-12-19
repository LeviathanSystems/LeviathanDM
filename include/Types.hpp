#ifndef TYPES_HPP
#define TYPES_HPP

// C++ standard library
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <cstddef>

// Wayland/wlroots types (with C++ compatibility handling)
#include "wayland/WaylandTypes.hpp"

namespace Leviathan {

// Forward declarations
class StatusBar;  // For Output

namespace Core {
    class Seat;
    class Screen;
    class Tag;
}

namespace Wayland {
    struct View;
    struct Output;
    class Server;
    class LayerManager;
}

// Layout types
enum class LayoutType {
    MASTER_STACK,
    MONOCLE,
    FLOATING,
    GRID
};

// Key modifier masks (using xkbcommon values)
enum Modifier {
    MOD_NONE = 0,
    MOD_SHIFT = (1 << 0),
    MOD_CTRL = (1 << 2),
    MOD_ALT = (1 << 3),
    MOD_SUPER = (1 << 6)
};

namespace Wayland {

// Forward declaration for Server
class Server;

// View (window) information structure
struct View {
    struct wlr_xdg_toplevel* xdg_toplevel;
    struct wlr_xdg_toplevel_decoration_v1* decoration;  // XDG decoration object
    struct wlr_surface* surface;
    struct wlr_scene_tree* scene_tree;
    Server* server;  // Reference to server for callbacks
    
    int x, y;
    int width, height;
    bool is_floating;
    bool is_fullscreen;
    bool mapped;
    
    struct wl_listener commit;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener decoration_request_mode;  // Decoration mode request
    
    View(struct wlr_xdg_toplevel* toplevel, Server* server);
    ~View();
};

// Output (monitor) information structure  
struct Output {
    struct wlr_output* wlr_output;
    struct wlr_scene_output* scene_output;  // Store scene output for wlr_scene_output_commit
    struct wl_listener frame;
    struct wl_listener destroy;
    struct wl_list link;
    Leviathan::Core::Screen* core_screen;  // Core screen object with EDID info
    Leviathan::Wayland::Server* server;  // Reference to compositor server
    Leviathan::Wayland::LayerManager* layer_manager;  // Per-output layer management
    
    Output(struct wlr_output* output, Leviathan::Wayland::Server* srv);
    ~Output();
};

} // namespace Wayland
} // namespace Leviathan

#endif // TYPES_HPP
