#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace Leviathan {
namespace UI {

/**
 * @brief Loads and caches application icons
 * 
 * Supports loading icons from:
 * - Absolute paths
 * - Icon names (searches icon theme directories)
 * - Supports PNG, SVG, and other formats via gdk-pixbuf
 */
class IconLoader {
public:
    IconLoader();
    ~IconLoader();
    
    /**
     * @brief Load an icon and return a Cairo surface
     * 
     * @param icon_name Icon name or path
     * @param size Desired icon size in pixels
     * @return Cairo surface with the icon, or nullptr if not found
     * 
     * The returned surface is owned by the IconLoader and cached.
     * Do not destroy it manually.
     */
    cairo_surface_t* LoadIcon(const std::string& icon_name, int size);
    
    /**
     * @brief Clear the icon cache
     */
    void ClearCache();
    
private:
    struct IconCacheKey {
        std::string name;
        int size;
        
        bool operator==(const IconCacheKey& other) const {
            return name == other.name && size == other.size;
        }
    };
    
    struct IconCacheKeyHash {
        std::size_t operator()(const IconCacheKey& key) const {
            return std::hash<std::string>()(key.name) ^ (std::hash<int>()(key.size) << 1);
        }
    };
    
    // Cache: icon_name+size -> cairo_surface
    std::unordered_map<IconCacheKey, cairo_surface_t*, IconCacheKeyHash> cache_;
    
    // Icon theme search paths
    std::vector<std::string> icon_theme_paths_;
    
    /**
     * @brief Find icon file in theme directories
     */
    std::string FindIconFile(const std::string& icon_name, int size);
    
    /**
     * @brief Load icon from file using gdk-pixbuf
     */
    cairo_surface_t* LoadIconFromFile(const std::string& filepath, int size);
};

} // namespace UI
} // namespace Leviathan
