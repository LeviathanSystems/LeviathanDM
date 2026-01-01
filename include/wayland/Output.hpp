#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "wayland/WaylandTypes.hpp"
#include "wayland/LayerManager.hpp"

// Forward declarations to avoid circular dependencies
namespace Leviathan {
namespace Core {
    class Screen;
}
}

namespace Leviathan {
namespace Wayland {

// Forward declarations
class Server;
class LayerManager;

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

class OutputManager {
public:
    static void HandleNewOutput(struct wl_listener* listener, void* data);
    static void HandleFrame(struct wl_listener* listener, void* data);
    static void HandleDestroy(struct wl_listener* listener, void* data);
};

} // namespace Wayland
} // namespace Leviathan

#endif // OUTPUT_HPP
