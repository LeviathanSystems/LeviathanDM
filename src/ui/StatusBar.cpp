#include "ui/StatusBar.hpp"
#include "wayland/LayerManager.hpp"
#include "Logger.hpp"
#include <ctime>
#include <cstring>

namespace Leviathan {

StatusBar::StatusBar(Wayland::LayerManager* layer_manager, int output_width)
    : layer_manager_(layer_manager),
      scene_rect_(nullptr),
      scene_buffer_(nullptr),
      buffer_(nullptr),
      cairo_surface_(nullptr),
      cairo_(nullptr),
      buffer_data_(nullptr),
      width_(output_width),
      height_(24),  // Status bar height
      current_workspace_(0),
      total_workspaces_(9),
      layout_name_("Tiling"),
      window_title_(""),
      time_str_("") {
    
    CreateSceneNodes();
    LOG_INFO("Created status bar: {}x{}", width_, height_);
}

StatusBar::~StatusBar() {
    if (cairo_) {
        cairo_destroy(cairo_);
    }
    if (cairo_surface_) {
        cairo_surface_destroy(cairo_surface_);
    }
    if (buffer_data_) {
        delete[] buffer_data_;
    }
    // Scene nodes are cleaned up automatically by wlroots
}

void StatusBar::CreateSceneNodes() {
    // Create status bar in the WorkingArea layer (where windows are)
    // This layer is for shells that can reserve space
    
    // Get the working area layer from the LayerManager
    auto* working_layer = layer_manager_->GetLayer(Wayland::Layer::WorkingArea);
    
    // Create a rectangle node for the status bar background
    float bg_color[4] = {
        static_cast<float>(bg_color_.r),
        static_cast<float>(bg_color_.g),
        static_cast<float>(bg_color_.b),
        0.95f  // Alpha
    };
    
    scene_rect_ = wlr_scene_rect_create(working_layer, width_, height_, bg_color);
    // Position at top of screen
    wlr_scene_node_set_position(&scene_rect_->node, 0, 0);
    // Raise to top of working area layer so it appears above windows
    wlr_scene_node_raise_to_top(&scene_rect_->node);
    
    // Reserve space in the LayerManager for the status bar
    Wayland::ReservedSpace reserved;
    reserved.top = height_;
    reserved.bottom = 0;
    reserved.left = 0;
    reserved.right = 0;
    layer_manager_->SetReservedSpace(reserved);
    
    LOG_DEBUG("Status bar created in WorkingArea layer, reserved {}px at top", height_);
}

void StatusBar::UpdateBuffer() {
    // TODO: Implement Cairo-to-wlr_buffer conversion
    // For now, the colored rectangle is sufficient to show the bar exists
}

void StatusBar::Render() {
    // Currently just showing the background rectangle
    // Text rendering will be added in next iteration
}

void StatusBar::DrawBackground() {
    // Handled by scene_rect_
}

void StatusBar::DrawWorkspaces() {
    // TODO: Use wlr_scene_text to render workspace numbers
}

void StatusBar::DrawLayout() {
    // TODO: Use wlr_scene_text to render layout name
}

void StatusBar::DrawTitle() {
    // TODO: Use wlr_scene_text to render window title
}

void StatusBar::DrawTime() {
    // TODO: Use wlr_scene_text to render time
}

void StatusBar::Update() {
    Render();
}

void StatusBar::UpdateTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%H:%M %d/%m/%Y", timeinfo);
    time_str_ = time_buffer;
    Render();
}

void StatusBar::SetWorkspace(int current, int total) {
    current_workspace_ = current;
    total_workspaces_ = total;
    Render();
}

void StatusBar::SetLayout(const std::string& layout_name) {
    layout_name_ = layout_name;
    Render();
}

void StatusBar::SetWindowTitle(const std::string& title) {
    window_title_ = title;
    Render();
}

} // namespace Leviathan
