#include "wayland/LayerSurface.hpp"
#include "wayland/Server.hpp"
#include "wayland/LayerManager.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace Wayland {

// C callback wrappers
static void layer_surface_handle_map(struct wl_listener* listener, void* data) {
    LayerSurfaceManager::HandleMap(listener, data);
}

static void layer_surface_handle_unmap(struct wl_listener* listener, void* data) {
    LayerSurfaceManager::HandleUnmap(listener, data);
}

static void layer_surface_handle_destroy(struct wl_listener* listener, void* data) {
    LayerSurfaceManager::HandleDestroy(listener, data);
}

static void layer_surface_handle_commit(struct wl_listener* listener, void* data) {
    LayerSurfaceManager::HandleCommit(listener, data);
}

static void layer_surface_handle_new_popup(struct wl_listener* listener, void* data) {
    LayerSurfaceManager::HandleNewPopup(listener, data);
}

void LayerSurfaceManager::HandleNewLayerSurface(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_layer_surface);
    struct wlr_layer_surface_v1* wlr_layer_surface = 
        static_cast<struct wlr_layer_surface_v1*>(data);
    
    LOG_INFO_FMT("New layer surface: namespace={}, layer={}", 
             wlr_layer_surface->namespace_ ? wlr_layer_surface->namespace_ : "null",
             (int)wlr_layer_surface->current.layer);
    
    // Create our layer surface wrapper
    LayerSurface* layer_surface = new LayerSurface();
    layer_surface->server = server;
    layer_surface->wlr_layer_surface = wlr_layer_surface;
    layer_surface->output = nullptr;
    
    // Setup listeners
    layer_surface->map.notify = layer_surface_handle_map;
    wl_signal_add(&wlr_layer_surface->surface->events.map, &layer_surface->map);
    
    layer_surface->unmap.notify = layer_surface_handle_unmap;
    wl_signal_add(&wlr_layer_surface->surface->events.unmap, &layer_surface->unmap);
    
    layer_surface->destroy.notify = layer_surface_handle_destroy;
    wl_signal_add(&wlr_layer_surface->surface->events.destroy, &layer_surface->destroy);
    
    layer_surface->commit.notify = layer_surface_handle_commit;
    wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer_surface->commit);
    
    // Note: new_popup would be handled here for xdg_popup support
    
    // Add to server's list
    wl_list_insert(&server->layer_surfaces, &layer_surface->link);
    
    // Assign output if not set (required for layer surfaces to work)
    if (!wlr_layer_surface->output) {
        struct wlr_output_layout* layout = server->GetOutputLayout();
        struct wlr_output* output = wlr_output_layout_output_at(layout, 0, 0);
        if (output) {
            wlr_layer_surface->output = output;
            layer_surface->output = output;
            LOG_DEBUG("Assigned output to layer surface");
        } else {
            LOG_ERROR("No output available for layer surface!");
        }
    }
    
    // Note: We don't send configure here! The initial configure must be sent
    // AFTER the first commit when the surface becomes initialized.
    // See HandleCommit() for the configure logic.
}

void LayerSurfaceManager::HandleMap(struct wl_listener* listener, void* data) {
    LayerSurface* layer_surface = wl_container_of(listener, layer_surface, map);
    
    LOG_INFO_FMT("Layer surface mapped: namespace={}", 
             layer_surface->wlr_layer_surface->namespace_ ? 
             layer_surface->wlr_layer_surface->namespace_ : "null");
    
    // Add to scene graph based on layer
    struct wlr_layer_surface_v1* wlr_ls = layer_surface->wlr_layer_surface;
    Server* server = layer_surface->server;
    
    // If no output assigned, use the first one from output layout
    struct wlr_output* wlr_output = wlr_ls->output;
    if (!wlr_output) {
        struct wlr_output_layout* layout = server->GetOutputLayout();
        wlr_output = wlr_output_layout_output_at(layout, 0, 0);
        if (wlr_output) {
            wlr_layer_surface_v1_configure(wlr_ls, wlr_output->width, wlr_output->height);
            LOG_DEBUG("Assigned layer surface to output");
        } else {
            LOG_WARN("No output available for layer surface!");
            return;
        }
    }
    
    // Find the Output struct for this wlr_output to get its LayerManager
    Output* output = server->FindOutput(wlr_output);
    
    if (!output || !output->layer_manager) {
        LOG_WARN("No Output or LayerManager found for layer surface");
        return;
    }
    
    // Determine which scene layer to use from this output's LayerManager
    struct wlr_scene_tree* parent_tree = nullptr;
    switch (wlr_ls->current.layer) {
        case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
            parent_tree = output->layer_manager->GetLayer(Layer::Background);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
            parent_tree = output->layer_manager->GetLayer(Layer::Background);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
            parent_tree = output->layer_manager->GetLayer(Layer::Top);
            break;
        case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
            parent_tree = output->layer_manager->GetLayer(Layer::Top);
            break;
    }
    
    if (parent_tree) {
        // Create scene layer surface
        layer_surface->scene_layer_surface = 
            wlr_scene_layer_surface_v1_create(parent_tree, wlr_ls);
        
        if (layer_surface->scene_layer_surface) {
            LOG_DEBUG("Layer surface added to scene graph");
        } else {
            LOG_ERROR("Failed to create scene layer surface!");
        }
    } else {
        LOG_ERROR("No parent tree found for layer surface!");
    }
    
    // Trigger arrangement
    if (wlr_ls->output) {
        layer_surface->output = wlr_ls->output;
        ArrangeLayer(wlr_ls->output, wlr_ls->current.layer);
    }
};

void LayerSurfaceManager::HandleUnmap(struct wl_listener* listener, void* data) {
    LayerSurface* layer_surface = wl_container_of(listener, layer_surface, unmap);
    
    LOG_INFO("Layer surface unmapped");
    
    // Scene node is automatically destroyed with the surface
    layer_surface->scene_layer_surface = nullptr;
}

void LayerSurfaceManager::HandleDestroy(struct wl_listener* listener, void* data) {
    LayerSurface* layer_surface = wl_container_of(listener, layer_surface, destroy);
    
    LOG_INFO("Layer surface destroyed");
    
    // Remove listeners
    wl_list_remove(&layer_surface->map.link);
    wl_list_remove(&layer_surface->unmap.link);
    wl_list_remove(&layer_surface->destroy.link);
    wl_list_remove(&layer_surface->commit.link);
    
    // Remove from server list
    wl_list_remove(&layer_surface->link);
    
    delete layer_surface;
}

void LayerSurfaceManager::HandleCommit(struct wl_listener* listener, void* data) {
    LayerSurface* layer_surface = wl_container_of(listener, layer_surface, commit);
    struct wlr_layer_surface_v1* wlr_ls = layer_surface->wlr_layer_surface;
    
    // Send initial configure on first commit after the surface is initialized
    // The surface becomes initialized after receiving its first commit from the client
    if (wlr_ls->initialized && wlr_ls->initial_commit) {
        if (wlr_ls->output) {
            uint32_t width = wlr_ls->current.desired_width;
            uint32_t height = wlr_ls->current.desired_height;
            
            // If client didn't specify size, use output size
            if (width == 0) width = wlr_ls->output->width;
            if (height == 0) height = wlr_ls->output->height;
            
            wlr_layer_surface_v1_configure(wlr_ls, width, height);
            LOG_DEBUG_FMT("Sent initial configure to layer surface: {}x{}", width, height);
        } else {
            LOG_ERROR("Layer surface initialized but has no output!");
        }
    }
    
    // Check if layer changed
    if (wlr_ls->current.committed & WLR_LAYER_SURFACE_V1_STATE_LAYER) {
        LOG_DEBUG("Layer surface layer changed");
        // Would need to move to different scene tree
    }
    
    // Rearrange if needed
    if (wlr_ls->output) {
        ArrangeLayer(wlr_ls->output, wlr_ls->current.layer);
    }
}

void LayerSurfaceManager::HandleNewPopup(struct wl_listener* listener, void* data) {
    // TODO: Handle xdg_popup for layer surfaces if needed
    LOG_DEBUG("Layer surface popup (not implemented yet)");
}

void LayerSurfaceManager::ArrangeLayer(struct wlr_output* output, 
                                       enum zwlr_layer_shell_v1_layer layer) {
    // This would implement the layer-shell exclusive zone logic
    // For now, wlroots scene graph handles basic positioning
    LOG_DEBUG_FMT("Arranging layer {} on output", (int)layer);
    
    // The scene layer surface automatically handles positioning based on
    // the layer surface's anchor and exclusive zone
}

} // namespace Wayland
} // namespace Leviathan
