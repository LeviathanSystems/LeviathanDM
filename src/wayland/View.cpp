#include "wayland/XwaylandCompat.hpp"  // Must be first to define wlr_xwayland_surface
#include "wayland/View.hpp"
#include "wayland/Output.hpp"  // For Output struct
#include "Logger.hpp"
#include "wayland/Server.hpp"
#include "config/ConfigParser.hpp"
#include "wayland/WaylandTypes.hpp"
#include <algorithm>
#include <cstdlib>

namespace Leviathan {
namespace Wayland {

// Forward declare static callbacks
static void view_handle_associate(struct wl_listener* listener, void* data);
static void view_handle_surface_destroy(struct wl_listener* listener, void* data);
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
    , xwayland_surface(nullptr)
    , decoration(nullptr)
    , surface(toplevel->base->surface)
    , scene_tree(nullptr)
    , server(srv)
    , is_xwayland(false)
    , border_top(nullptr)
    , border_right(nullptr)
    , border_bottom(nullptr)
    , border_left(nullptr)
    , shadow_top(nullptr)
    , shadow_right(nullptr)
    , shadow_bottom(nullptr)
    , shadow_left(nullptr)
    , x(0), y(0)
    , width(0), height(0)
    , is_floating(false)
    , is_fullscreen(false)
    , mapped(false)
    , opacity(1.0f)
    , border_radius(0) {
    
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

// Xwayland constructor
View::View(struct ::wlr_xwayland_surface* xwayland_surf, Server* srv)
    : xdg_toplevel(nullptr)
    , xwayland_surface(xwayland_surf)
    , decoration(nullptr)
    , surface(xwayland_surface_get_surface(xwayland_surf))
    , scene_tree(nullptr)
    , server(srv)
    , is_xwayland(true)
    , border_top(nullptr)
    , border_right(nullptr)
    , border_bottom(nullptr)
    , border_left(nullptr)
    , shadow_top(nullptr)
    , shadow_right(nullptr)
    , shadow_bottom(nullptr)
    , shadow_left(nullptr)
    , x(xwayland_surface_get_x(xwayland_surf)), y(xwayland_surface_get_y(xwayland_surf))
    , width(xwayland_surface_get_width(xwayland_surf)), height(xwayland_surface_get_height(xwayland_surf))
    , is_floating(false)
    , is_fullscreen(false)
    , mapped(false)
    , opacity(1.0f)
    , border_radius(0) {
    
    // For XWayland surfaces, only add listeners for events that exist immediately
    // Surface-related events (commit, map, unmap) require waiting for the associate event
    
    // These events exist immediately on the xwayland_surface itself
    destroy.notify = view_handle_destroy;
    wl_signal_add(xwayland_surface_get_events_destroy(xwayland_surf), &destroy);
    
    request_move.notify = view_handle_request_move;
    wl_signal_add(xwayland_surface_get_events_request_move(xwayland_surf), &request_move);
    
    request_resize.notify = view_handle_request_resize;
    wl_signal_add(xwayland_surface_get_events_request_resize(xwayland_surf), &request_resize);
    
    request_maximize.notify = view_handle_request_maximize;
    wl_signal_add(xwayland_surface_get_events_request_maximize(xwayland_surf), &request_maximize);
    
    request_fullscreen.notify = view_handle_request_fullscreen;
    wl_signal_add(xwayland_surface_get_events_request_fullscreen(xwayland_surf), &request_fullscreen);
    
    // Register the associate event listener to add surface listeners when wl_surface becomes available
    associate.notify = view_handle_associate;
    wl_signal_add(xwayland_surface_get_events_associate(xwayland_surf), &associate);
    
    // These events require a wl_surface, which might not exist yet
    // Only add them if the surface is already associated
    if (surface) {
        commit.notify = view_handle_commit;
        wl_signal_add(&surface->events.commit, &commit);
        
        map.notify = view_handle_map;
        wl_signal_add(&surface->events.map, &map);
        
        unmap.notify = view_handle_unmap;
        wl_signal_add(&surface->events.unmap, &unmap);
        
        // Add surface destroy listener
        surface_destroy.notify = view_handle_surface_destroy;
        wl_signal_add(&surface->events.destroy, &surface_destroy);
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "XWayland view created with surface already associated");
    } else {
        // Initialize the wl_list links so wl_list_remove() in destructor is safe
        wl_list_init(&commit.link);
        wl_list_init(&map.link);
        wl_list_init(&unmap.link);
        wl_list_init(&surface_destroy.link);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "XWayland view created, waiting for surface association");
    }
}

View::~View() {
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "View destructor called for {}", static_cast<void*>(this));
    
    // Clean up borders and shadows
    DestroyBorders();
    DestroyShadows();
    
    // DON'T call RemoveView from destructor - it causes use-after-free!
    // The destroy callback (view_handle_destroy) should handle cleanup BEFORE deleting
    
    // Remove wayland listeners
    wl_list_remove(&commit.link);
    wl_list_remove(&map.link);
    wl_list_remove(&unmap.link);
    wl_list_remove(&destroy.link);
    wl_list_remove(&request_move.link);
    wl_list_remove(&request_resize.link);
    wl_list_remove(&request_maximize.link);
    wl_list_remove(&request_fullscreen.link);
    
    // Remove X11-specific listeners
    if (is_xwayland) {
        wl_list_remove(&associate.link);
        wl_list_remove(&surface_destroy.link);
    }
}

// Handler for wl_surface destroy (X11 windows only)
// This is called when the wl_surface is destroyed, which happens BEFORE the xwayland_surface is destroyed
// We need to remove our listeners from the surface before it's destroyed
static void view_handle_surface_destroy(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, surface_destroy);
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "wl_surface destroyed for X11 view={}, removing surface listeners", 
                 static_cast<void*>(view));
    
    // Remove listeners that were attached to the wl_surface
    wl_list_remove(&view->commit.link);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->surface_destroy.link);
    
    // Re-initialize the links so destructor doesn't crash
    wl_list_init(&view->commit.link);
    wl_list_init(&view->map.link);
    wl_list_init(&view->unmap.link);
    wl_list_init(&view->surface_destroy.link);
    
    view->surface = nullptr;
}

void view_handle_associate(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, associate);
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "XWayland surface associated! view={}, now adding surface listeners", 
                 static_cast<void*>(view));
    
    // Now the wl_surface is available, so we can add the surface-related listeners
    view->surface = xwayland_surface_get_surface(view->xwayland_surface);
    
    if (view->surface) {
        // IMPORTANT: Remove the initialized list links first to avoid double-linking
        wl_list_remove(&view->commit.link);
        wl_list_remove(&view->map.link);
        wl_list_remove(&view->unmap.link);
        
        // Add commit listener
        view->commit.notify = view_handle_commit;
        wl_signal_add(&view->surface->events.commit, &view->commit);
        
        // Add map listener
        view->map.notify = view_handle_map;
        wl_signal_add(&view->surface->events.map, &view->map);
        
        // Add unmap listener
        view->unmap.notify = view_handle_unmap;
        wl_signal_add(&view->surface->events.unmap, &view->unmap);
        
        // Add surface destroy listener to clean up when wl_surface is destroyed
        view->surface_destroy.notify = view_handle_surface_destroy;
        wl_signal_add(&view->surface->events.destroy, &view->surface_destroy);
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Added commit, map, unmap, and surface_destroy listeners to XWayland surface");
        
        // Check if the surface is already mapped (can happen with X11 windows)
        // If so, we need to manually trigger the map handler
        if (wlr_surface_has_buffer(view->surface)) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Surface already has buffer, manually triggering map handler");
            view_handle_map(&view->map, view->surface);
        }
    } else {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Associate event fired but surface is still NULL!");
    }
}

static void view_handle_commit(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, commit);
    
    // XWayland surfaces don't use XDG shell protocol, so skip XDG-specific handling
    if (view->is_xwayland) {
        // X11 windows don't need configure events - they manage their own size
        // Just handle size updates if needed
        if (view->border_top && view->server) {
            int surface_width = xwayland_surface_get_width(view->xwayland_surface);
            int surface_height = xwayland_surface_get_height(view->xwayland_surface);
            
            // Only update if the surface size has actually changed
            if (surface_width > 0 && surface_height > 0 &&
                (surface_width != view->width || surface_height != view->height)) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "X11 surface size changed from {}x{} to {}x{}, updating borders",
                             view->width, view->height, surface_width, surface_height);
                
                // Update view dimensions to match surface
                view->width = surface_width;
                view->height = surface_height;
            }
        }
        return;
    }
    
    // XDG toplevel handling
    //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Commit handler called! view={}, xdg_toplevel={}, base={}", 
    //          static_cast<void*>(view),
    //          static_cast<void*>(view->xdg_toplevel),
    //          static_cast<void*>(view->xdg_toplevel->base));
    //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  initial_commit={}, initialized={}, mapped={}", 
    //          view->xdg_toplevel->base->initial_commit,
    //          view->xdg_toplevel->base->initialized,
    //          view->mapped);
    
    // Check if this is the initial commit
    // The compositor MUST send a configure event in response to initial_commit
    // or the client will never map the surface
    if (view->xdg_toplevel->base->initial_commit) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Initial commit received for view={}, sending configure", 
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
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Set decoration mode to SERVER_SIDE (no client decorations)");
            } else {
                wlr_xdg_toplevel_decoration_v1_set_mode(view->decoration,
                    WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Set decoration mode to CLIENT_SIDE (client draws decorations)");
            }
        }
    } else {
        // After initial commit, check if surface size changed and update borders
        if (view->border_top && view->server) {
            auto* surface = view->xdg_toplevel->base->surface;
            int surface_width = surface->current.width;
            int surface_height = surface->current.height;
            
            // Only update if the surface size has actually changed
            if (surface_width > 0 && surface_height > 0 &&
                (surface_width != view->width || surface_height != view->height)) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Surface size changed from {}x{} to {}x{}, updating borders",
                             view->width, view->height, surface_width, surface_height);
                
                // Update view dimensions to match surface
                view->width = surface_width;
                view->height = surface_height;
                
                // Note: Border updates are now handled by window decoration system
            }
        }
    }
}

static void view_handle_map(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, map);
    view->mapped = true;
    
    std::string app_id;
    std::string title;
    
    if (view->is_xwayland) {
        // X11 window
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "X11 View mapped! view={}, xwayland_surface={}", 
                 static_cast<void*>(view),
                 static_cast<void*>(view->xwayland_surface));
        
        app_id = XWAYLAND_CLASS(view->xwayland_surface) ? XWAYLAND_CLASS(view->xwayland_surface) : "";
        title = XWAYLAND_TITLE(view->xwayland_surface) ? XWAYLAND_TITLE(view->xwayland_surface) : "";
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "X11 window mapped - class='{}', title='{}'", app_id, title);
        
        // Create scene tree for X11 window now that it's mapped
        if (!view->scene_tree && view->server) {
            struct wlr_scene_tree* parent_layer = &view->server->GetScene()->tree;
            
            // Find first output's WorkingArea layer
            Output* first_output = view->server->GetFirstOutput();
            if (first_output && first_output->layer_manager) {
                parent_layer = first_output->layer_manager->GetLayer(Layer::WorkingArea);
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Adding X11 window to WorkingArea layer");
            }
            
            view->scene_tree = wlr_scene_subsurface_tree_create(parent_layer, view->surface);
            if (view->scene_tree) {
                view->scene_tree->node.data = view;
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Created scene tree for mapped X11 window");
            }
        }
    } else {
        // Wayland native (XDG) window
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "View mapped! view={}, xdg_toplevel={}", 
                 static_cast<void*>(view),
                 static_cast<void*>(view->xdg_toplevel));
        
        app_id = view->xdg_toplevel->app_id ? view->xdg_toplevel->app_id : "";
        title = view->xdg_toplevel->title ? view->xdg_toplevel->title : "";
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Window mapped - app_id='{}', title='{}'", app_id, title);
    }
    
    // Apply window decorations based on rules (works for both XDG and X11)
    const auto* rule = Leviathan::Config().window_rules.FindMatch(
        app_id, title, "", view->is_floating
    );
    
    if (rule) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Matched window rule: '{}' for app_id='{}'", rule->name, app_id);
        
        // Apply decoration group if specified
        if (!rule->decoration_group.empty()) {
            const auto* decoration = Leviathan::Config().window_decorations.FindByName(
                rule->decoration_group
            );
            
            if (decoration) {
                view->ApplyDecorationConfig(*decoration, true);  // true = focused
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Applied decoration group '{}' to window", rule->decoration_group);
            } else {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "Decoration group '{}' not found", rule->decoration_group);
            }
        }
        
        // Apply other rule actions
        if (rule->force_floating && !view->is_floating) {
            view->is_floating = true;
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Forced window '{}' to float", app_id);
        }
        if (rule->force_tiled && view->is_floating) {
            view->is_floating = false;
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Forced window '{}' to tile", app_id);
        }
        if (rule->opacity_override.has_value()) {
            float opacity = rule->opacity_override.value() / 100.0f;
            view->SetOpacity(opacity);
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Overrode opacity to {}%", rule->opacity_override.value());
        }
    } else {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "No matching rule found for app_id='{}', title='{}'", app_id, title);
        
        // Apply default decoration if available
        const auto* default_decoration = Leviathan::Config().window_decorations.GetDefault();
        if (default_decoration) {
            view->ApplyDecorationConfig(*default_decoration, true);
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Applied default decoration");
        }
    }
    
    // Ensure the view is visible by raising it in the scene graph
    if (view->scene_tree) {
        wlr_scene_node_raise_to_top(&view->scene_tree->node);
        wlr_scene_node_set_enabled(&view->scene_tree->node, true);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Raised scene tree to top and enabled it");
    }
    
    // Give keyboard focus to the newly mapped view
    if (view->server) {
        view->server->FocusView(view);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Gave keyboard focus to newly mapped view");
        
        // Trigger auto-tiling on the focused screen's LayerManager
        // This will create borders via MoveResizeView after setting dimensions
        auto* focused_screen = view->server->GetFocusedScreen();
        if (focused_screen) {
            auto* layer_mgr = view->server->GetLayerManagerForScreen(focused_screen);
            if (layer_mgr) {
                layer_mgr->AutoTile();
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Triggered auto-tile after view mapped");
            }
        }
    }
}

static void view_handle_unmap(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "View unmapped! view={}", static_cast<void*>(view));
    
    // Trigger auto-tiling to reorganize remaining views
    if (view->server) {
        auto* focused_screen = view->server->GetFocusedScreen();
        if (focused_screen) {
            auto* layer_mgr = view->server->GetLayerManagerForScreen(focused_screen);
            if (layer_mgr) {
                layer_mgr->AutoTile();
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Triggered auto-tile after view unmapped");
            }
        }
    }
}

static void view_handle_destroy(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, destroy);
    
    // Do cleanup BEFORE deleting the view
    if (view->server) {
        view->server->RemoveView(view);
    }
    
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
    
    if (view->is_xwayland) {
        // XWayland fullscreen handling
        // TODO: Implement XWayland fullscreen support
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "XWayland fullscreen request (not yet implemented)");
        return;
    }
    
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

void View::CreateBorders(int border_width, const float color[4]) {
    if (!scene_tree || border_width <= 0) {
        return;
    }
    
    // Destroy existing borders first
    DestroyBorders();
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "CreateBorders: view={}, width={}, height={}, border_width={}", 
                  static_cast<void*>(this), width, height, border_width);
    
    // Create 4 border rectangles around the view
    // Use scene_tree->node.parent to add borders as siblings, not children
    auto* parent = scene_tree->node.parent;
    if (!parent) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "Cannot create borders: scene_tree has no parent");
        return;
    }
    
    // Top border
    border_top = wlr_scene_rect_create(parent, width + 2 * border_width, border_width, color);
    wlr_scene_node_set_position(&border_top->node, -border_width, -border_width);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  Created top border: size={}x{}, pos=({},{})", 
                  width + 2 * border_width, border_width, -border_width, -border_width);
    
    // Right border
    border_right = wlr_scene_rect_create(parent, border_width, height, color);
    wlr_scene_node_set_position(&border_right->node, width, 0);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  Created right border: size={}x{}, pos=({},{})", 
                  border_width, height, width, 0);
    
    // Bottom border
    border_bottom = wlr_scene_rect_create(parent, width + 2 * border_width, border_width, color);
    wlr_scene_node_set_position(&border_bottom->node, -border_width, height);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  Created bottom border: size={}x{}, pos=({},{})", 
                  width + 2 * border_width, border_width, -border_width, height);
    
    // Left border
    border_left = wlr_scene_rect_create(parent, border_width, height, color);
    wlr_scene_node_set_position(&border_left->node, -border_width, 0);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  Created left border: size={}x{}, pos=({},{})", 
                  border_width, height, -border_width, 0);
}

void View::UpdateBorderColor(const float color[4]) {
    if (border_top) wlr_scene_rect_set_color(border_top, color);
    if (border_right) wlr_scene_rect_set_color(border_right, color);
    if (border_bottom) wlr_scene_rect_set_color(border_bottom, color);
    if (border_left) wlr_scene_rect_set_color(border_left, color);
}

void View::UpdateBorderSize(int border_width) {
    if (!scene_tree || border_width <= 0) {
        DestroyBorders();
        return;
    }
    
    if (border_top) {
        wlr_scene_rect_set_size(border_top, width + 2 * border_width, border_width);
        wlr_scene_node_set_position(&border_top->node, -border_width, -border_width);
    }
    
    if (border_right) {
        wlr_scene_rect_set_size(border_right, border_width, height);
        wlr_scene_node_set_position(&border_right->node, width, 0);
    }
    
    if (border_bottom) {
        wlr_scene_rect_set_size(border_bottom, width + 2 * border_width, border_width);
        wlr_scene_node_set_position(&border_bottom->node, -border_width, height);
    }
    
    if (border_left) {
        wlr_scene_rect_set_size(border_left, border_width, height);
        wlr_scene_node_set_position(&border_left->node, -border_width, 0);
    }
}

void View::DestroyBorders() {
    if (border_top) {
        wlr_scene_node_destroy(&border_top->node);
        border_top = nullptr;
    }
    if (border_right) {
        wlr_scene_node_destroy(&border_right->node);
        border_right = nullptr;
    }
    if (border_bottom) {
        wlr_scene_node_destroy(&border_bottom->node);
        border_bottom = nullptr;
    }
    if (border_left) {
        wlr_scene_node_destroy(&border_left->node);
        border_left = nullptr;
    }
}

void View::SetOpacity(float new_opacity) {
    opacity = std::max(0.0f, std::min(1.0f, new_opacity));
    
    if (!scene_tree) {
        return;
    }
    
    // Apply opacity to all child nodes in the scene tree
    // Note: wlr_scene_buffer_set_opacity requires iterating through scene buffers
    struct wlr_scene_node* node;
    wl_list_for_each(node, &scene_tree->children, link) {
        if (node->type == WLR_SCENE_NODE_BUFFER) {
            struct wlr_scene_buffer* scene_buffer = wlr_scene_buffer_from_node(node);
            wlr_scene_buffer_set_opacity(scene_buffer, opacity);
        }
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Set window opacity to {}", opacity);
}

void View::SetBorderRadius(int radius) {
    border_radius = std::max(0, radius);
    
    // Note: Border radius rendering requires custom shader or corner masks
    // This is a placeholder - full implementation would need:
    // 1. Corner mask textures
    // 2. Custom scene graph nodes
    // 3. Or use client-side decorations with rounded corners
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Set border radius to {} (rendering not yet implemented)", border_radius);
}

void View::CreateShadows(int shadow_size, const float color[4], float shadow_opacity) {
    if (!scene_tree || shadow_size <= 0) {
        return;
    }
    
    // Destroy existing shadows first
    DestroyShadows();
    
    // Create shadow color with opacity
    float shadow_color[4] = {
        color[0],
        color[1],
        color[2],
        color[3] * shadow_opacity
    };
    
    // Use scene_tree->node.parent to add shadows as siblings
    auto* parent = scene_tree->node.parent;
    if (!parent) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "Cannot create shadows: scene_tree has no parent");
        return;
    }
    
    // Create shadow rectangles (larger than borders, positioned behind window)
    // Top shadow
    shadow_top = wlr_scene_rect_create(parent, width + 2 * shadow_size, shadow_size, shadow_color);
    wlr_scene_node_set_position(&shadow_top->node, -shadow_size, -shadow_size);
    wlr_scene_node_lower_to_bottom(&shadow_top->node);
    
    // Right shadow
    shadow_right = wlr_scene_rect_create(parent, shadow_size, height, shadow_color);
    wlr_scene_node_set_position(&shadow_right->node, width, 0);
    wlr_scene_node_lower_to_bottom(&shadow_right->node);
    
    // Bottom shadow
    shadow_bottom = wlr_scene_rect_create(parent, width + 2 * shadow_size, shadow_size, shadow_color);
    wlr_scene_node_set_position(&shadow_bottom->node, -shadow_size, height);
    wlr_scene_node_lower_to_bottom(&shadow_bottom->node);
    
    // Left shadow
    shadow_left = wlr_scene_rect_create(parent, shadow_size, height, shadow_color);
    wlr_scene_node_set_position(&shadow_left->node, -shadow_size, 0);
    wlr_scene_node_lower_to_bottom(&shadow_left->node);
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Created shadows with size={}, opacity={}", shadow_size, shadow_opacity);
}

void View::DestroyShadows() {
    if (shadow_top) {
        wlr_scene_node_destroy(&shadow_top->node);
        shadow_top = nullptr;
    }
    if (shadow_right) {
        wlr_scene_node_destroy(&shadow_right->node);
        shadow_right = nullptr;
    }
    if (shadow_bottom) {
        wlr_scene_node_destroy(&shadow_bottom->node);
        shadow_bottom = nullptr;
    }
    if (shadow_left) {
        wlr_scene_node_destroy(&shadow_left->node);
        shadow_left = nullptr;
    }
}

void View::ApplyDecorationConfig(const Leviathan::WindowDecorationConfig& config, bool is_focused) {
    // Apply opacity
    float target_opacity = is_focused ? config.opacity : config.opacity_inactive;
    SetOpacity(target_opacity);
    
    // Apply border
    float border_color[4];
    const std::string& color_str = is_focused ? config.border_color_focused : config.border_color_unfocused;
    Leviathan::ConfigParser::HexToRGBA(color_str, border_color);
    
    if (config.border_width > 0) {
        CreateBorders(config.border_width, border_color);
    } else {
        DestroyBorders();
    }
    
    // Apply border radius (note: rendering not yet fully implemented)
    SetBorderRadius(config.border_radius);
    
    // Apply shadows
    if (config.enable_shadows) {
        float shadow_color[4];
        Leviathan::ConfigParser::HexToRGBA(config.shadow_color, shadow_color);
        CreateShadows(config.shadow_size, shadow_color, config.shadow_opacity);
    } else {
        DestroyShadows();
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Applied decoration to window: opacity={}, border_width={}, border_radius={}",
                  target_opacity, config.border_width, config.border_radius);
}

} // namespace Wayland
} // namespace Leviathan
