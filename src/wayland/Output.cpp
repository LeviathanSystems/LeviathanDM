#include "wayland/Output.hpp"
#include "Types.hpp"
#include "Logger.hpp"
#include "wayland/Server.hpp"
#include "wayland/LayerManager.hpp"
#include "core/Seat.hpp"

extern "C" {
#include <wlr/types/wlr_output.h>
#include <stdlib.h>
#include <time.h>
}

namespace Leviathan {
namespace Wayland {

Output::Output(struct wlr_output* output, Server* srv)
    : wlr_output(output), scene_output(nullptr), core_screen(nullptr), server(srv), layer_manager(nullptr) {
}

Output::~Output() {
    // Remove screen from core seat before deleting
    if (core_screen && server) {
        auto* core_seat = server->GetCoreSeat();
        if (core_seat) {
            core_seat->RemoveScreen(core_screen);
            LOG_INFO_FMT("Removed screen '{}' from core seat", core_screen->GetName());
        }
    }
    
    // Clean up layer manager (wallpaper cleanup is handled in LayerManager destructor)
    if (layer_manager) {
        delete layer_manager;
    }
    
    if (core_screen) {
        delete core_screen;
    }
}

void OutputManager::HandleFrame(struct wl_listener* listener, void* data) {
    // Frame callback - render and notify clients
    Output* output = wl_container_of(listener, output, frame);
    
    if (!output->scene_output) {
        LOG_ERROR("scene_output is NULL in frame handler!");
        return;
    }
    
    // Render the scene if needed and commit the output
    if (!wlr_scene_output_commit(output->scene_output, nullptr)) {
        // Scene is clean, nothing needs to be rendered
        // Still need to send frame_done to keep clients updated
    }
    
    // CRITICAL: Send frame_done to all surfaces so they know we're ready for next frame
    // Without this, clients will render once and then freeze waiting for us
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(output->scene_output, &now);
}

void OutputManager::HandleDestroy(struct wl_listener* listener, void* data) {
    Output* output = wl_container_of(listener, output, destroy);
    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    delete output;
}

void OutputManager::HandleNewOutput(struct wl_listener* listener, void* data) {
    // This is handled in Server::OnNewOutput
}

} // namespace Wayland
} // namespace Leviathan
