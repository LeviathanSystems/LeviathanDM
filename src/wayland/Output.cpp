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
    // Frame callback - use wlr_scene_output_commit which handles everything
    Output* output = wl_container_of(listener, output, frame);
    
    // Log EVERY frame to diagnose the issue
    static int frame_count = 0;
    frame_count++;
    LOG_INFO("Frame handler called! (count={}) for output {}", 
              frame_count, output->wlr_output->name);
    
    if (!output->scene_output) {
        LOG_ERROR("scene_output is NULL in frame handler!");
        return;
    }
    
    // wlr_scene_output_commit handles rendering, committing, and frame done events
    // Pass NULL for default options
    if (!wlr_scene_output_commit(output->scene_output, nullptr)) {
        LOG_ERROR("Failed to commit scene output for {}", output->wlr_output->name);
        return;
    }
    
    LOG_DEBUG("Successfully committed scene output");
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
