#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cairo.h>
#include "wayland/WaylandTypes.hpp"
#include "config/ConfigParser.hpp"
#include "ui/Widget.hpp"

namespace Leviathan {

// Forward declarations
namespace Wayland {
    class Server;
    class LayerManager;
}

namespace UI {
    class WidgetPlugin;
}

class StatusBar {
public:
    StatusBar(const StatusBarConfig& config,
              Wayland::LayerManager* layer_manager, 
              uint32_t output_width,
              uint32_t output_height);
    ~StatusBar();

    void Render();
    void Update();
    
    // Get the reserved space this bar needs
    int GetReservedSize() const;
    StatusBarConfig::Position GetPosition() const { return config_.position; }
    
    int GetHeight() const;
    int GetWidth() const;

private:
    void CreateSceneNodes();
    void CreateWidgets();
    void RenderToBuffer();
    void UploadToTexture();
    
    StatusBarConfig config_;
    Wayland::LayerManager* layer_manager_;
    
    struct wlr_scene_rect* scene_rect_;      // Background rectangle
    struct wlr_scene_buffer* scene_buffer_;  // For rendered widgets
    struct wlr_texture* texture_;            // GPU texture
    struct wlr_renderer* renderer_;          // Renderer for texture upload
    
    int pos_x_, pos_y_;  // Position on screen
    
    // Cairo rendering
    cairo_surface_t* cairo_surface_;
    cairo_t* cairo_;
    uint32_t* buffer_data_;
    
    // Dimensions
    uint32_t output_width_;
    uint32_t output_height_;
    int bar_width_;   // Actual bar width
    int bar_height_;  // Actual bar height
    
    // Widget containers - raw pointers (ownership managed below)
    std::vector<UI::Widget*> left_widgets_;
    std::vector<UI::Widget*> center_widgets_;
    std::vector<UI::Widget*> right_widgets_;
    
    // Ownership of built-in widgets
    std::vector<std::unique_ptr<UI::Widget>> owned_widgets_;
    
    // Ownership of plugin widgets (keeps them alive)
    std::vector<std::shared_ptr<UI::WidgetPlugin>> plugin_widgets_;
};

} // namespace Leviathan
