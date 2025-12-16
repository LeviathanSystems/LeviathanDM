#include "wayland/Output.hpp"
#include "Types.hpp"
#include "Logger.hpp"

extern "C" {
#include <wlr/types/wlr_output.h>
#include <stdlib.h>
#include <time.h>
}

namespace Leviathan {
namespace Wayland {

Output::Output(struct wlr_output* output)
    : wlr_output(output), scene_output(nullptr) {
}

Output::~Output() {
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
