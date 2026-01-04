#ifndef WAYLAND_VIEW_HPP
#define WAYLAND_VIEW_HPP

#include "wayland/WaylandTypes.hpp"
#include "wayland/XwaylandCompat.hpp"
#include "config/ConfigParser.hpp"  // For WindowDecorationConfig

namespace Leviathan {
namespace Wayland {

// Forward declarations
class Server;

// View (window) information structure
struct View {
    // Surface type union - either XDG or Xwayland
    struct wlr_xdg_toplevel* xdg_toplevel;
    struct ::wlr_xwayland_surface* xwayland_surface;  // Use global namespace for C types
    
    struct wlr_xdg_toplevel_decoration_v1* decoration;  // XDG decoration object
    struct wlr_surface* surface;
    struct wlr_scene_tree* scene_tree;
    Server* server;  // Reference to server for callbacks
    
    bool is_xwayland;  // True if this is an X11 window
    
    // Border rectangles (top, right, bottom, left)
    struct wlr_scene_rect* border_top;
    struct wlr_scene_rect* border_right;
    struct wlr_scene_rect* border_bottom;
    struct wlr_scene_rect* border_left;
    
    // Shadow rectangles (for drop shadow effect)
    struct wlr_scene_rect* shadow_top;
    struct wlr_scene_rect* shadow_right;
    struct wlr_scene_rect* shadow_bottom;
    struct wlr_scene_rect* shadow_left;
    
    int x, y;
    int width, height;
    bool is_floating;
    bool is_fullscreen;
    bool mapped;
    float opacity;  // Current window opacity (0.0 - 1.0)
    int border_radius;  // Border radius in pixels
    
    struct wl_listener commit;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener surface_destroy;  // wl_surface destroy (for X11 windows to clean up listeners)
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
    struct wl_listener decoration_request_mode;  // Decoration mode request
    struct wl_listener associate;  // XWayland surface association (when wl_surface becomes available)
    
    // Constructors for different surface types
    View(struct wlr_xdg_toplevel* toplevel, Server* server);
    View(struct ::wlr_xwayland_surface* xwayland_surface, Server* server);  // Use global namespace for C types
    ~View();
    
    // Border management
    void CreateBorders(int border_width, const float color[4]);
    void UpdateBorderColor(const float color[4]);
    void UpdateBorderSize(int border_width);
    void DestroyBorders();
    
    // Styling
    void SetOpacity(float opacity);
    void SetBorderRadius(int radius);
    void CreateShadows(int shadow_size, const float color[4], float opacity);
    void DestroyShadows();
    void ApplyDecorationConfig(const WindowDecorationConfig& config, bool is_focused);
};

class ViewManager {
public:
    static void HandleNewXdgSurface(struct wl_listener* listener, void* data);
    static void HandleCommit(struct wl_listener* listener, void* data);
    static void HandleMap(struct wl_listener* listener, void* data);
    static void HandleUnmap(struct wl_listener* listener, void* data);
    static void HandleDestroy(struct wl_listener* listener, void* data);
    static void HandleRequestMove(struct wl_listener* listener, void* data);
    static void HandleRequestResize(struct wl_listener* listener, void* data);
    static void HandleRequestMaximize(struct wl_listener* listener, void* data);
    static void HandleRequestFullscreen(struct wl_listener* listener, void* data);
};

} // namespace Wayland
} // namespace Leviathan

#endif // WAYLAND_VIEW_HPP
