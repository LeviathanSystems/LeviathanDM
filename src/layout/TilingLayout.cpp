#include "layout/TilingLayout.hpp"
#include "Logger.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
}

#include <cmath>
#include <algorithm>

namespace Leviathan {

using Wayland::View;

TilingLayout::TilingLayout() {
}

void TilingLayout::ApplyMasterStack(std::vector<View*>& views,
                                   int master_count, float master_ratio,
                                   int screen_width, int screen_height,
                                   int gap_size) {
    if (views.empty()) {
        return;
    }
    
    int n = views.size();
    master_count = std::min(master_count, n);
    
    if (n == 1) {
        // Single window - fullscreen in workspace area
        MoveResizeView(views[0], 
                      gap_size, gap_size,
                      screen_width - 2 * gap_size, 
                      screen_height - 2 * gap_size);
        return;
    }
    
    if (master_count == n) {
        // All windows are masters - split vertically
        int window_height = (screen_height - (n + 1) * gap_size) / n;
        
        for (int i = 0; i < n; ++i) {
            int y = gap_size + i * (window_height + gap_size);
            MoveResizeView(views[i],
                          gap_size, y,
                          screen_width - 2 * gap_size,
                          window_height);
        }
        return;
    }
    
    // Master-stack layout
    int master_width = screen_width * master_ratio;
    int stack_width = screen_width - master_width - 3 * gap_size;
    int stack_count = n - master_count;
    
    // Position master windows
    int master_height = (screen_height - (master_count + 1) * gap_size) / master_count;
    
    for (int i = 0; i < master_count; ++i) {
        int y = gap_size + i * (master_height + gap_size);
        MoveResizeView(views[i],
                      gap_size, y,
                      master_width - gap_size,
                      master_height);
    }
    
    // Position stack windows
    if (stack_count > 0) {
        int stack_height = (screen_height - (stack_count + 1) * gap_size) / stack_count;
        
        for (int i = 0; i < stack_count; ++i) {
            int y = gap_size + i * (stack_height + gap_size);
            int x = master_width + 2 * gap_size;
            
            MoveResizeView(views[master_count + i],
                          x, y,
                          stack_width,
                          stack_height);
        }
    }
}

void TilingLayout::ApplyMonocle(std::vector<View*>& views,
                               int screen_width, int screen_height) {
    // All windows fullscreen in workspace area, stacked on top of each other
    for (auto* view : views) {
        MoveResizeView(view, 0, 0, screen_width, screen_height);
    }
}

void TilingLayout::ApplyGrid(std::vector<View*>& views,
                            int screen_width, int screen_height,
                            int gap_size) {
    if (views.empty()) {
        return;
    }
    
    int n = views.size();
    
    // Calculate grid dimensions
    int cols = std::max(1, static_cast<int>(std::ceil(std::sqrt(n))));
    int rows = std::max(1, static_cast<int>(std::ceil(static_cast<float>(n) / cols)));
    
    int window_width = (screen_width - (cols + 1) * gap_size) / cols;
    int window_height = (screen_height - (rows + 1) * gap_size) / rows;
    
    for (int i = 0; i < n; ++i) {
        int row = i / cols;
        int col = i % cols;
        
        int x = gap_size + col * (window_width + gap_size);
        int y = gap_size + row * (window_height + gap_size);
        
        MoveResizeView(views[i], x, y, window_width, window_height);
    }
}

void TilingLayout::MoveResizeView(View* view,
                                 int x, int y,
                                 int width, int height) {
    LOG_DEBUG_FMT("MoveResizeView: view={}, pos=({},{}), size=({},{}), scene_tree={}", 
              static_cast<void*>(view), x, y, width, height, 
              static_cast<void*>(view->scene_tree));
    
    view->x = x;
    view->y = y;
    view->width = width;
    view->height = height;
    
    if (view->scene_tree) {
        wlr_scene_node_set_position(&view->scene_tree->node, x, y);
        LOG_DEBUG("  - Set scene node position");
    } else {
        LOG_DEBUG("  - WARNING: scene_tree is NULL!");
    }
    
    wlr_xdg_toplevel_set_size(view->xdg_toplevel, width, height);
    LOG_DEBUG("  - Set toplevel size");
}

} // namespace Leviathan
