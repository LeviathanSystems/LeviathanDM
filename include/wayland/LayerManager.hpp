#ifndef LAYER_MANAGER_HPP
#define LAYER_MANAGER_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include "config/ConfigParser.hpp"

// Forward declarations
struct wlr_scene;
struct wlr_scene_tree;
struct wlr_scene_node;
struct wlr_output;
struct wl_event_loop;
struct wl_event_source;

namespace Leviathan {

// Forward declarations
namespace Core {
    class Tag;
    class Client;
}

// Forward declarations
class TilingLayout;
struct StatusBarsConfig;
class StatusBar;
class ShmBuffer;
enum class LayoutType;

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
    LayerManager(struct wlr_scene* scene, struct wlr_output* output, struct wl_event_loop* event_loop);
    ~LayerManager();
    
    // Set monitor configuration (includes wallpaper config)
    void SetMonitorConfig(const MonitorConfig& config);
    
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
    
    // Tag management (per-screen workspaces)
    void InitializeTags(const std::vector<TagConfig>& tag_configs);
    void SwitchToTag(int index);
    Core::Tag* GetCurrentTag();
    std::vector<Core::Tag*> GetTags() const;
    int GetCurrentTagIndex() const { return current_tag_index_; }
    
    // Layout management for current tag
    void SetLayout(LayoutType layout);
    void IncreaseMasterCount();
    void DecreaseMasterCount();
    void IncreaseMasterRatio();
    void DecreaseMasterRatio();
    
    // Client management
    void AddView(class View* view);  // Add view and auto-tile
    void RemoveView(class View* view);  // Remove view and auto-tile
    void MoveClientToTag(Core::Client* client, int target_tag_index);
    
    // Auto-tile current tag's views
    void AutoTile();
    
private:
    struct wlr_scene_tree* layers_[static_cast<size_t>(Layer::COUNT)];
    ReservedSpace reserved_space_;
    struct wlr_output* output_;  // The output this manager belongs to
    struct wl_event_loop* event_loop_;  // For timers and events
    std::vector<Leviathan::StatusBar*> status_bars_;  // Status bars on this output
    
    // Per-screen tags (workspaces)
    std::vector<std::unique_ptr<Core::Tag>> tags_;
    int current_tag_index_ = 0;
    
    // Layout engine for tiling
    TilingLayout* layout_engine_ = nullptr;
    
    // Monitor configuration
    const MonitorConfig* monitor_config_ = nullptr;
    
    // Wallpaper state (managed internally)
    struct wlr_scene_node* wallpaper_node_ = nullptr;
    class ShmBuffer* wallpaper_buffer_ = nullptr;
    std::string current_wallpaper_path_;
    size_t wallpaper_index_ = 0;
    std::vector<std::string> wallpaper_paths_;
    struct wl_event_source* wallpaper_timer_ = nullptr;
    
    // Wallpaper helpers (private)
    class ShmBuffer* LoadWallpaperImage(const std::string& path, int width, int height);
    void InitializeWallpaper();
    void ClearWallpaper();
    void NextWallpaper();
    static int WallpaperRotationCallback(void* data);
};

} // namespace Wayland
} // namespace Leviathan

#endif // LAYER_MANAGER_HPP
