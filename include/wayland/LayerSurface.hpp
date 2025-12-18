#ifndef LAYER_SURFACE_HPP
#define LAYER_SURFACE_HPP

#include "wayland/WaylandTypes.hpp"
#include "Types.hpp"

// Layer shell protocol uses 'namespace' which is a C++ keyword
// The wlroots wrapper already uses 'namespace_' to avoid conflict
#define namespace namespace_
extern "C" {
#include <wlr/types/wlr_layer_shell_v1.h>
}
#undef namespace

namespace Leviathan {
namespace Wayland {

// Forward declaration
class Server;

struct LayerSurface {
    Server* server;
    struct wlr_layer_surface_v1* wlr_layer_surface;
    struct wlr_scene_layer_surface_v1* scene_layer_surface;
    
    // Listeners
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener new_popup;
    
    // Output this layer surface is on
    struct wlr_output* output;
    
    // Link in Server's layer_surfaces list
    struct wl_list link;
};

class LayerSurfaceManager {
public:
    static void HandleNewLayerSurface(struct wl_listener* listener, void* data);
    static void HandleMap(struct wl_listener* listener, void* data);
    static void HandleUnmap(struct wl_listener* listener, void* data);
    static void HandleDestroy(struct wl_listener* listener, void* data);
    static void HandleCommit(struct wl_listener* listener, void* data);
    static void HandleNewPopup(struct wl_listener* listener, void* data);
    
    // Arrange layer surfaces on an output
    static void ArrangeLayer(struct wlr_output* output, 
                            enum zwlr_layer_shell_v1_layer layer);
};

} // namespace Wayland
} // namespace Leviathan

#endif // LAYER_SURFACE_HPP
