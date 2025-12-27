#ifndef WALLPAPER_MANAGER_HPP
#define WALLPAPER_MANAGER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "config/ConfigParser.hpp"

// Forward declarations
struct wlr_scene_tree;
struct wlr_scene_node;
struct wlr_output;
struct wl_event_loop;
struct wl_event_source;

namespace Leviathan {

// Forward declarations
class ShmBuffer;

namespace Wayland {

/**
 * WallpaperManager - Manages wallpaper rendering and rotation for an output
 * 
 * This class handles:
 * - Loading and scaling wallpaper images
 * - Rendering wallpapers to a scene layer
 * - Automatic wallpaper rotation based on configuration
 */
class WallpaperManager {
public:
    /**
     * Constructor
     * @param background_layer The scene tree layer to render wallpapers to
     * @param event_loop The Wayland event loop for timers
     * @param width Width of the wallpaper area
     * @param height Height of the wallpaper area
     */
    WallpaperManager(struct wlr_scene_tree* background_layer, struct wl_event_loop* event_loop,
                     int width, int height);
    
    /**
     * Destructor - cleans up wallpaper resources
     */
    ~WallpaperManager();
    
    /**
     * Set monitor configuration and initialize wallpaper
     * @param config Monitor configuration containing wallpaper settings
     */
    void SetMonitorConfig(const MonitorConfig& config);
    
    /**
     * Clear the current wallpaper
     */
    void ClearWallpaper();
    
    /**
     * Switch to the next wallpaper in rotation
     */
    void NextWallpaper();
    
    /**
     * Get the current wallpaper path
     */
    const std::string& GetCurrentWallpaperPath() const { return current_wallpaper_path_; }
    
    /**
     * Check if wallpaper is currently active
     */
    bool HasWallpaper() const { return wallpaper_node_ != nullptr; }

private:
    // Scene layer for wallpaper rendering
    struct wlr_scene_tree* background_layer_;
    
    // Event loop for timers
    struct wl_event_loop* event_loop_;
    
    // Wallpaper dimensions
    int width_;
    int height_;
    
    // Monitor configuration
    const MonitorConfig* monitor_config_ = nullptr;
    
    // Current wallpaper type
    WallpaperType current_type_ = WallpaperType::StaticImage;
    
    // Static image wallpaper state
    struct wlr_scene_node* wallpaper_node_ = nullptr;
    class ShmBuffer* wallpaper_buffer_ = nullptr;
    
    // WallpaperEngine support disabled
    // std::unique_ptr<WallpaperEngineRenderer> we_renderer_;
    
    // Common state
    std::string current_wallpaper_path_;
    size_t wallpaper_index_ = 0;
    std::vector<std::string> wallpaper_paths_;
    struct wl_event_source* wallpaper_timer_ = nullptr;
    
    // Wallpaper helpers (private)
    /**
     * Load and scale a wallpaper image to fit the output
     * @param path Path to the wallpaper image file
     * @param width Target width (output width)
     * @param height Target height (output height)
     * @return ShmBuffer containing the scaled image, or nullptr on failure
     */
    class ShmBuffer* LoadWallpaperImage(const std::string& path, int width, int height);
    
    /**
     * Initialize wallpaper from configuration
     */
    void InitializeWallpaper();
    
    /**
     * Initialize a static image wallpaper
     */
    void InitializeStaticWallpaper(const std::string& wallpaper_path);
    
    /**
     * WallpaperEngine support disabled
     * Initialize a WallpaperEngine wallpaper
     */
    // void InitializeWallpaperEngine(const std::string& wallpaper_path);
    
    /**
     * Static callback for wallpaper rotation timer
     */
    static int WallpaperRotationCallback(void* data);
};

} // namespace Wayland
} // namespace Leviathan

#endif // WALLPAPER_MANAGER_HPP
