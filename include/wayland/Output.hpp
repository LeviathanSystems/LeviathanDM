#ifndef OUTPUT_HPP
#define OUTPUT_HPP

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
}

namespace Leviathan {
namespace Wayland {

class Server;

class OutputManager {
public:
    static void HandleNewOutput(struct wl_listener* listener, void* data);
    static void HandleFrame(struct wl_listener* listener, void* data);
    static void HandleDestroy(struct wl_listener* listener, void* data);
};

} // namespace Wayland
} // namespace Leviathan

#endif // OUTPUT_HPP
