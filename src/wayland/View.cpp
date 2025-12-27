#include "wayland/View.hpp"
#include "Logger.hpp"
#include "wayland/Server.hpp"
#include "config/ConfigParser.hpp"
#include "wayland/WaylandTypes.hpp"
#include <algorithm>
#include <cstdlib>

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

View::~View() {
    LOG_DEBUG_FMT("View destructor called for {}", static_cast<void*>(this));
    
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
}

static void view_handle_commit(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, commit);
    
    //LOG_DEBUG_FMT("Commit handler called! view={}, xdg_toplevel={}, base={}", 
    //          static_cast<void*>(view),
    //          static_cast<void*>(view->xdg_toplevel),
    //          static_cast<void*>(view->xdg_toplevel->base));
    //LOG_DEBUG_FMT("  initial_commit={}, initialized={}, mapped={}", 
    //          view->xdg_toplevel->base->initial_commit,
    //          view->xdg_toplevel->base->initialized,
    //          view->mapped);
    
    // Check if this is the initial commit
    // The compositor MUST send a configure event in response to initial_commit
    // or the client will never map the surface
    if (view->xdg_toplevel->base->initial_commit) {
        LOG_INFO_FMT("Initial commit received for view={}, sending configure", 
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
        // After initial commit, check if surface size changed and update borders
        if (view->border_top && view->server) {
            auto* surface = view->xdg_toplevel->base->surface;
            int surface_width = surface->current.width;
            int surface_height = surface->current.height;
            
            // Only update if the surface size has actually changed
            if (surface_width > 0 && surface_height > 0 &&
                (surface_width != view->width || surface_height != view->height)) {
                LOG_DEBUG_FMT("Surface size changed from {}x{} to {}x{}, updating borders",
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
    LOG_INFO_FMT("View mapped! view={}, xdg_toplevel={}", 
             static_cast<void*>(view),
             static_cast<void*>(view->xdg_toplevel));
    
    // Apply window decorations based on rules
    if (view->xdg_toplevel) {
        std::string app_id = view->xdg_toplevel->app_id ? view->xdg_toplevel->app_id : "";
        std::string title = view->xdg_toplevel->title ? view->xdg_toplevel->title : "";
        
        LOG_DEBUG_FMT("Window mapped - app_id='{}', title='{}'", app_id, title);
        
        // Find matching window rule
        const auto* rule = Leviathan::Config().window_rules.FindMatch(
            app_id, title, "", view->is_floating
        );
        
        if (rule) {
            LOG_INFO_FMT("Matched window rule: '{}' for app_id='{}'", rule->name, app_id);
            
            // Apply decoration group if specified
            if (!rule->decoration_group.empty()) {
                const auto* decoration = Leviathan::Config().window_decorations.FindByName(
                    rule->decoration_group
                );
                
                if (decoration) {
                    view->ApplyDecorationConfig(*decoration, true);  // true = focused
                    LOG_INFO_FMT("Applied decoration group '{}' to window", rule->decoration_group);
                } else {
                    LOG_WARN_FMT("Decoration group '{}' not found", rule->decoration_group);
                }
            }
            
            // Apply other rule actions
            if (rule->force_floating && !view->is_floating) {
                view->is_floating = true;
                LOG_DEBUG_FMT("Forced window '{}' to float", app_id);
            }
            if (rule->force_tiled && view->is_floating) {
                view->is_floating = false;
                LOG_DEBUG_FMT("Forced window '{}' to tile", app_id);
            }
            if (rule->opacity_override.has_value()) {
                float opacity = rule->opacity_override.value() / 100.0f;
                view->SetOpacity(opacity);
                LOG_DEBUG_FMT("Overrode opacity to {}%", rule->opacity_override.value());
            }
        } else {
            LOG_DEBUG_FMT("No matching rule found for app_id='{}', title='{}'", app_id, title);
            
            // Apply default decoration if available
            const auto* default_decoration = Leviathan::Config().window_decorations.GetDefault();
            if (default_decoration) {
                view->ApplyDecorationConfig(*default_decoration, true);
                LOG_DEBUG("Applied default decoration");
            }
        }
    }
    
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
        
        // Trigger auto-tiling on the focused screen's LayerManager
        // This will create borders via MoveResizeView after setting dimensions
        auto* focused_screen = view->server->GetFocusedScreen();
        if (focused_screen) {
            auto* layer_mgr = view->server->GetLayerManagerForScreen(focused_screen);
            if (layer_mgr) {
                layer_mgr->AutoTile();
                LOG_DEBUG("Triggered auto-tile after view mapped");
            }
        }
    }
}

static void view_handle_unmap(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, unmap);
    view->mapped = false;
    LOG_INFO_FMT("View unmapped! view={}", static_cast<void*>(view));
    
    // Trigger auto-tiling to reorganize remaining views
    if (view->server) {
        auto* focused_screen = view->server->GetFocusedScreen();
        if (focused_screen) {
            auto* layer_mgr = view->server->GetLayerManagerForScreen(focused_screen);
            if (layer_mgr) {
                layer_mgr->AutoTile();
                LOG_DEBUG("Triggered auto-tile after view unmapped");
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
    
    LOG_DEBUG_FMT("CreateBorders: view={}, width={}, height={}, border_width={}", 
                  static_cast<void*>(this), width, height, border_width);
    
    // Create 4 border rectangles around the view
    // Use scene_tree->node.parent to add borders as siblings, not children
    auto* parent = scene_tree->node.parent;
    if (!parent) {
        LOG_WARN("Cannot create borders: scene_tree has no parent");
        return;
    }
    
    // Top border
    border_top = wlr_scene_rect_create(parent, width + 2 * border_width, border_width, color);
    wlr_scene_node_set_position(&border_top->node, -border_width, -border_width);
    LOG_DEBUG_FMT("  Created top border: size={}x{}, pos=({},{})", 
                  width + 2 * border_width, border_width, -border_width, -border_width);
    
    // Right border
    border_right = wlr_scene_rect_create(parent, border_width, height, color);
    wlr_scene_node_set_position(&border_right->node, width, 0);
    LOG_DEBUG_FMT("  Created right border: size={}x{}, pos=({},{})", 
                  border_width, height, width, 0);
    
    // Bottom border
    border_bottom = wlr_scene_rect_create(parent, width + 2 * border_width, border_width, color);
    wlr_scene_node_set_position(&border_bottom->node, -border_width, height);
    LOG_DEBUG_FMT("  Created bottom border: size={}x{}, pos=({},{})", 
                  width + 2 * border_width, border_width, -border_width, height);
    
    // Left border
    border_left = wlr_scene_rect_create(parent, border_width, height, color);
    wlr_scene_node_set_position(&border_left->node, -border_width, 0);
    LOG_DEBUG_FMT("  Created left border: size={}x{}, pos=({},{})", 
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
    
    LOG_DEBUG_FMT("Set window opacity to {}", opacity);
}

void View::SetBorderRadius(int radius) {
    border_radius = std::max(0, radius);
    
    // Note: Border radius rendering requires custom shader or corner masks
    // This is a placeholder - full implementation would need:
    // 1. Corner mask textures
    // 2. Custom scene graph nodes
    // 3. Or use client-side decorations with rounded corners
    
    LOG_DEBUG_FMT("Set border radius to {} (rendering not yet implemented)", border_radius);
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
        LOG_WARN("Cannot create shadows: scene_tree has no parent");
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
    
    LOG_DEBUG_FMT("Created shadows with size={}, opacity={}", shadow_size, shadow_opacity);
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
    
    LOG_DEBUG_FMT("Applied decoration to window: opacity={}, border_width={}, border_radius={}",
                  target_opacity, config.border_width, config.border_radius);
}

} // namespace Wayland
} // namespace Leviathan
