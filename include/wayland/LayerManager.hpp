#ifndef LAYER_MANAGER_HPP
#define LAYER_MANAGER_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

// Forward declarations
struct wlr_scene;
struct wlr_scene_tree;
struct wlr_output;

namespace Leviathan {

// Forward declarations
class TilingLayout;
struct StatusBarsConfig;
class StatusBar;

namespace Core {
    class Tag;
}

namespace Wayland {

// Forward declaration
class Server;

/**
 * Layer ordering (bottom to top):
 * - Background: Wallpaper, solid colors
 * - Working Area: Regular application windows and bars
 *   - Bars take reserved space from working area
 *   - Applications tile in remaining space
 * - Top: Scratchpads, notifications, overlays
 */
enum class Layer {
    Background = 0,
    WorkingArea = 1,
    Top = 2,
    COUNT = 3
};

/**
 * Reserved space for bars/panels
 * This reduces the usable working area for applications
 */
struct ReservedSpace {
    uint32_t top = 0;     // Pixels reserved at top
    uint32_t bottom = 0;  // Pixels reserved at bottom
    uint32_t left = 0;    // Pixels reserved at left
    uint32_t right = 0;   // Pixels reserved at right
};

/**
 * Usable area for tiling windows
 */
struct UsableArea {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

class LayerManager {
public:
    LayerManager(struct wlr_scene* scene, struct wlr_output* output);
    ~LayerManager();
    
    // Get scene tree for a specific layer
    struct wlr_scene_tree* GetLayer(Layer layer);
    
    // Reserve space on edges (for bars/panels)
    void SetReservedSpace(const ReservedSpace& space);
    const ReservedSpace& GetReservedSpace() const { return reserved_space_; }
    
    // Calculate usable area for applications
    // Takes output geometry and subtracts reserved space
    UsableArea CalculateUsableArea(int32_t output_x, int32_t output_y,
                                   uint32_t output_width, uint32_t output_height) const;
    
    // Convenience methods
    struct wlr_scene_tree* GetBackgroundLayer() { return GetLayer(Layer::Background); }
    struct wlr_scene_tree* GetWorkingAreaLayer() { return GetLayer(Layer::WorkingArea); }
    struct wlr_scene_tree* GetTopLayer() { return GetLayer(Layer::Top); }
    
    // Get the output this manager belongs to
    struct wlr_output* GetOutput() const { return output_; }
    
    // Tile windows in this output's working area
    // Takes a list of views that should be tiled according to the tag's layout
    void TileViews(std::vector<class View*>& views, 
                   Core::Tag* tag,
                   TilingLayout* layout_engine);
    
    // Status bar management
    void AddStatusBar(Leviathan::StatusBar* bar);
    void RemoveStatusBar(Leviathan::StatusBar* bar);
    const std::vector<Leviathan::StatusBar*>& GetStatusBars() const { return status_bars_; }
    
    // Create status bars from configuration
    // Takes a list of status bar names and the full config
    void CreateStatusBars(const std::vector<std::string>& bar_names,
                         const StatusBarsConfig& all_bars_config,
                         uint32_t output_width,
                         uint32_t output_height);
    
private:
    struct wlr_scene_tree* layers_[static_cast<size_t>(Layer::COUNT)];
    ReservedSpace reserved_space_;
    struct wlr_output* output_;  // The output this manager belongs to
    std::vector<Leviathan::StatusBar*> status_bars_;  // Status bars on this output
};

} // namespace Wayland
} // namespace Leviathan

#endif // LAYER_MANAGER_HPP
