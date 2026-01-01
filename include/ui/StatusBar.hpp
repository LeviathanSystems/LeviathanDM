#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cairo.h>
#include "wayland/WaylandTypes.hpp"
#include "config/ConfigParser.hpp"
#include "ui/reusable-widgets/Container.hpp"
#include "ui/reusable-widgets/HBox.hpp"
#include "ui/reusable-widgets/VBox.hpp"
#include "ui/ShmBuffer.hpp"

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
              struct wl_event_loop* event_loop,
              uint32_t output_width,
              uint32_t output_height);
    ~StatusBar();

    void Render();
    void Update();
    
    // Check if any widgets need re-rendering
    void CheckDirtyWidgets();
    
    // Get the reserved space this bar needs
    int GetReservedSize() const;
    StatusBarConfig::Position GetPosition() const { return config_.position; }
    
    int GetHeight() const;
    int GetWidth() const;
    
    // Mouse event handling
    bool HandleClick(int x, int y);
    bool HandleHover(int x, int y);
    
    // Get bar bounds for input region setup
    void GetBounds(int& x, int& y, int& width, int& height) const {
        x = pos_x_; y = pos_y_;
        width = bar_width_; height = bar_height_;
    }
    
    // Get root container for popover search
    std::shared_ptr<UI::Container> GetRootContainer() const { return root_container_; }

private:
    void CreateSceneNodes();
    void CreateWidgets();
    void RenderToBuffer();
    void UploadToTexture();
    void SetupDirtyCheckTimer();
    
    // Static callback for timer
    static int OnDirtyCheckTimer(void* data);
    
    StatusBarConfig config_;
    Wayland::LayerManager* layer_manager_;
    struct wl_event_loop* event_loop_;
    struct wl_event_source* dirty_check_timer_;
    
    struct wlr_scene_rect* scene_rect_;      // Background rectangle
    struct wlr_scene_buffer* scene_buffer_;  // For rendered widgets
    struct wlr_texture* texture_;            // GPU texture
    struct wlr_renderer* renderer_;          // Renderer for texture upload
    ShmBuffer* shm_buffer_;                  // Custom SHM buffer implementation
    bool buffer_attached_;                   // Track if buffer is attached to scene
    
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
    
    // Layout system - use Container/HBox containers for automatic layout
    std::shared_ptr<UI::Container> root_container_;  // Root container (can be HBox or VBox)
    std::shared_ptr<UI::HBox> left_container_;       // Left section (legacy)
    std::shared_ptr<UI::HBox> center_container_;     // Center section (legacy)
    std::shared_ptr<UI::HBox> right_container_;      // Right section (legacy)
    
    // Ownership of built-in widgets
    std::vector<std::unique_ptr<UI::Widget>> owned_widgets_;
    
    // Ownership of plugin widgets (keeps them alive)
    std::vector<std::shared_ptr<UI::WidgetPlugin>> plugin_widgets_;
};

} // namespace Leviathan
