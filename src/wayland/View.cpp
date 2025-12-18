#include "wayland/View.hpp"
#include "Logger.hpp"
#include "wayland/Server.hpp"
#include "config/ConfigParser.hpp"
#include <algorithm>

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include <stdlib.h>
}

namespace Leviathan {
namespace Wayland {

// Forward declare static callbacks
static void view_handle_commit(struct wl_listener* listener, void* data);
static void view_handle_map(struct wl_listener* listener, void* data);
static void view_handle_unmap(struct wl_listener* listener, void* data);
static void view_handle_destroy(struct wl_listener* listener, void* data);
static void view_handle_request_move(struct wl_listener* listener, void* data);
static void view_handle_request_resize(struct wl_listener* listener, void* data);
static void view_handle_request_maximize(struct wl_listener* listener, void* data);
static void view_handle_request_fullscreen(struct wl_listener* listener, void* data);

View::View(struct wlr_xdg_toplevel* toplevel, Server* srv)
    : xdg_toplevel(toplevel)
    , decoration(nullptr)
    , surface(toplevel->base->surface)
    , scene_tree(nullptr)
    , server(srv)
    , x(0), y(0)
    , width(0), height(0)
    , is_floating(false)
    , is_fullscreen(false)
    , mapped(false) {
    
    // Setup commit listener - CRITICAL for initial configure
    commit.notify = view_handle_commit;
    wl_signal_add(&xdg_toplevel->base->surface->events.commit, &commit);
    
    // Setup listeners
    map.notify = view_handle_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &map);
    
    unmap.notify = view_handle_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &unmap);
    
    destroy.notify = view_handle_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &destroy);
    
    request_move.notify = view_handle_request_move;
    wl_signal_add(&xdg_toplevel->events.request_move, &request_move);
    
    request_resize.notify = view_handle_request_resize;
    wl_signal_add(&xdg_toplevel->events.request_resize, &request_resize);
    
    request_maximize.notify = view_handle_request_maximize;
    wl_signal_add(&xdg_toplevel->events.request_maximize, &request_maximize);
    
    request_fullscreen.notify = view_handle_request_fullscreen;
    wl_signal_add(&xdg_toplevel->events.request_fullscreen, &request_fullscreen);
}

View::~View() {
    LOG_DEBUG("View destructor called for {}", static_cast<void*>(this));
    
    // Let server handle the cleanup (removing from lists, retiling, etc.)
    if (server) {
        server->RemoveView(this);
    }
    
    // Remove wayland listeners
    wl_list_remove(&commit.link);
    wl_list_remove(&map.link);
    wl_list_remove(&unmap.link);
    wl_list_remove(&destroy.link);
    wl_list_remove(&request_move.link);
    wl_list_remove(&request_resize.link);
    wl_list_remove(&request_maximize.link);
    wl_list_remove(&request_fullscreen.link);
}

static void view_handle_commit(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, commit);
    
    //LOG_DEBUG("Commit handler called! view={}, xdg_toplevel={}, base={}", 
    //          static_cast<void*>(view),
    //          static_cast<void*>(view->xdg_toplevel),
    //          static_cast<void*>(view->xdg_toplevel->base));
    //LOG_DEBUG("  initial_commit={}, initialized={}, mapped={}", 
    //          view->xdg_toplevel->base->initial_commit,
    //          view->xdg_toplevel->base->initialized,
    //          view->mapped);
    
    // Check if this is the initial commit
    // The compositor MUST send a configure event in response to initial_commit
    // or the client will never map the surface
    if (view->xdg_toplevel->base->initial_commit) {
        LOG_INFO("Initial commit received for view={}, sending configure", 
                 static_cast<void*>(view));
        
        // Disable client-side decorations (no titlebar)
        wlr_xdg_toplevel_set_tiled(view->xdg_toplevel, 
                                   WLR_EDGE_TOP | WLR_EDGE_BOTTOM | 
                                   WLR_EDGE_LEFT | WLR_EDGE_RIGHT);
        
        // Send configure with size 0,0 to let the client pick its own dimensions
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, 0, 0);
        
        // If we have a decoration object, check config and set mode accordingly
        if (view->decoration) {
            // Check config to see if we should remove client titlebars
            bool remove_titlebars = Config().general.remove_client_titlebars;
            
            if (remove_titlebars) {
                wlr_xdg_toplevel_decoration_v1_set_mode(view->decoration,
                    WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
                LOG_INFO("Set decoration mode to SERVER_SIDE (no client decorations)");
            } else {
                wlr_xdg_toplevel_decoration_v1_set_mode(view->decoration,
                    WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
                LOG_INFO("Set decoration mode to CLIENT_SIDE (client draws decorations)");
            }
        }
    } else {
        //LOG_DEBUG("Not initial commit, skipping configure");
    }
}

static void view_handle_map(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, map);
    view->mapped = true;
    LOG_INFO("View mapped! view={}, xdg_toplevel={}", 
             static_cast<void*>(view),
             static_cast<void*>(view->xdg_toplevel));
    
    // Ensure the view is visible by raising it in the scene graph
    if (view->scene_tree) {
        wlr_scene_node_raise_to_top(&view->scene_tree->node);
        wlr_scene_node_set_enabled(&view->scene_tree->node, true);
        LOG_DEBUG("Raised scene tree to top and enabled it");
    }
    
    // Give keyboard focus to the newly mapped view
    if (view->server) {
        view->server->FocusView(view);
        LOG_DEBUG("Gave keyboard focus to newly mapped view");
    }
    
    // Trigger tiling layout update
    if (view->server) {
        view->server->TileViews();
    }
}

static void view_handle_unmap(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    LOG_INFO("View unmapped! view={}", static_cast<void*>(view));
    
    // Trigger tiling layout update when view is unmapped
    if (view->server) {
        view->server->TileViews();
    }
}

static void view_handle_destroy(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, destroy);
    delete view;
}

static void view_handle_request_move(struct wl_listener* listener, void* data) {
    // Handle interactive move (for future implementation)
}

static void view_handle_request_resize(struct wl_listener* listener, void* data) {
    // Handle interactive resize (for future implementation)
}

static void view_handle_request_maximize(struct wl_listener* listener, void* data) {
    // Handle maximize request
}

static void view_handle_request_fullscreen(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, request_fullscreen);
    struct wlr_xdg_toplevel* toplevel = static_cast<struct wlr_xdg_toplevel*>(data);
    
    view->is_fullscreen = toplevel->requested.fullscreen;
    wlr_xdg_toplevel_set_fullscreen(view->xdg_toplevel, toplevel->requested.fullscreen);
}

void ViewManager::HandleNewXdgSurface(struct wl_listener* listener, void* data) {
    // This is handled in Server::OnNewXdgSurface
}

void ViewManager::HandleMap(struct wl_listener* listener, void* data) {
    view_handle_map(listener, data);
}

void ViewManager::HandleUnmap(struct wl_listener* listener, void* data) {
    view_handle_unmap(listener, data);
}

void ViewManager::HandleDestroy(struct wl_listener* listener, void* data) {
    view_handle_destroy(listener, data);
}

void ViewManager::HandleRequestMove(struct wl_listener* listener, void* data) {
    view_handle_request_move(listener, data);
}

void ViewManager::HandleRequestResize(struct wl_listener* listener, void* data) {
    view_handle_request_resize(listener, data);
}

void ViewManager::HandleRequestMaximize(struct wl_listener* listener, void* data) {
    view_handle_request_maximize(listener, data);
}

void ViewManager::HandleRequestFullscreen(struct wl_listener* listener, void* data) {
    view_handle_request_fullscreen(listener, data);
}

} // namespace Wayland
} // namespace Leviathan
