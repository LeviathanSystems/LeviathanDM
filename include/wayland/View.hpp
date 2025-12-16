#ifndef WAYLAND_VIEW_HPP
#define WAYLAND_VIEW_HPP

#include "Types.hpp"

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
}

namespace Leviathan {
namespace Wayland {

class Server;

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
