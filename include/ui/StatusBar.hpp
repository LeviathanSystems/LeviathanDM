#pragma once

#include <string>
#include <memory>
#include <cairo.h>
#include "wayland/WaylandTypes.hpp"

namespace Leviathan {

// Forward declarations
namespace Wayland {
    class Server;
    class LayerManager;
}

class StatusBar {
public:
    StatusBar(Wayland::LayerManager* layer_manager, int output_width);
    ~StatusBar();

    void Render();
    void Update();
    void SetWorkspace(int current, int total);
    void SetLayout(const std::string& layout_name);
    void SetWindowTitle(const std::string& title);
    void UpdateTime();
    
    struct wlr_scene_rect* GetSceneRect() { return scene_rect_; }
    struct wlr_scene_buffer* GetSceneBuffer() { return scene_buffer_; }
    int GetHeight() const { return height_; }

private:
    void CreateSceneNodes();
    void UpdateBuffer();
    void DrawBackground();
    void DrawWorkspaces();
    void DrawLayout();
    void DrawTitle();
    void DrawTime();
    
    Wayland::LayerManager* layer_manager_;
    struct wlr_scene_rect* scene_rect_;      // Background rectangle
    struct wlr_scene_buffer* scene_buffer_;  // For text/content
    struct wlr_buffer* buffer_;
    
    // Cairo rendering
    cairo_surface_t* cairo_surface_;
    cairo_t* cairo_;
    uint32_t* buffer_data_;
    
    // Dimensions
    int width_;
    int height_;
    
    // Status info
    int current_workspace_;
    int total_workspaces_;
    std::string layout_name_;
    std::string window_title_;
    std::string time_str_;
    
    // Colors (RGB)
    struct Color {
        double r, g, b;
    };
    Color bg_color_ = {0.12, 0.12, 0.12};      // #1e1e1e
    Color fg_color_ = {0.9, 0.9, 0.9};         // #e6e6e6
    Color accent_color_ = {0.37, 0.51, 0.67};  // #5e81ac
    Color inactive_color_ = {0.4, 0.4, 0.4};   // #666666
};

} // namespace Leviathan
