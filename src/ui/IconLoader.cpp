#include "ui/IconLoader.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <cstdlib>
#include <sstream>

namespace Leviathan {
namespace UI {

IconLoader::IconLoader() {
    // Standard icon theme directories (following XDG spec)
    icon_theme_paths_.push_back("/usr/share/icons");
    icon_theme_paths_.push_back("/usr/share/pixmaps");
    icon_theme_paths_.push_back("/usr/local/share/icons");
    
    // User local icons
    const char* home = getenv("HOME");
    if (home) {
        icon_theme_paths_.push_back(std::string(home) + "/.local/share/icons");
        icon_theme_paths_.push_back(std::string(home) + "/.icons");
    }
    
    // XDG_DATA_DIRS
    const char* xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs) {
        std::stringstream ss(xdg_data_dirs);
        std::string path;
        while (std::getline(ss, path, ':')) {
            if (!path.empty()) {
                icon_theme_paths_.push_back(path + "/icons");
            }
        }
    }
}

IconLoader::~IconLoader() {
    ClearCache();
}

void IconLoader::ClearCache() {
    for (auto& pair : cache_) {
        if (pair.second) {
            cairo_surface_destroy(pair.second);
        }
    }
    cache_.clear();
}

cairo_surface_t* IconLoader::LoadIcon(const std::string& icon_name, int size) {
    if (icon_name.empty()) {
        return nullptr;
    }
    
    // Check cache first
    IconCacheKey key{icon_name, size};
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    
    // Find icon file
    std::string icon_path;
    
    // Check if it's an absolute path
    if (icon_name[0] == '/' && std::filesystem::exists(icon_name)) {
        icon_path = icon_name;
    } else {
        // Search in icon theme directories
        icon_path = FindIconFile(icon_name, size);
    }
    
    if (icon_path.empty()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Icon not found: {}", icon_name);
        cache_[key] = nullptr;
        return nullptr;
    }
    
    // Load the icon
    cairo_surface_t* surface = LoadIconFromFile(icon_path, size);
    cache_[key] = surface;
    
    return surface;
}

std::string IconLoader::FindIconFile(const std::string& icon_name, int size) {
    // Common extensions to try
    std::vector<std::string> extensions = {".png", ".svg", ".xpm", ".jpg", ".jpeg"};
    
    // If icon_name already has extension, try it directly
    if (icon_name.find('.') != std::string::npos) {
        for (const auto& base_path : icon_theme_paths_) {
            std::string path = base_path + "/" + icon_name;
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
    }
    
    // Search in theme directories
    // Priority: hicolor theme with exact size match
    std::vector<std::string> size_dirs = {
        "hicolor/" + std::to_string(size) + "x" + std::to_string(size) + "/apps",
        "hicolor/" + std::to_string(size) + "x" + std::to_string(size) + "/places",
        "hicolor/" + std::to_string(size) + "x" + std::to_string(size) + "/categories",
        "hicolor/scalable/apps",
        "hicolor/scalable/places"
    };
    
    for (const auto& base_path : icon_theme_paths_) {
        // Try size-specific directories
        for (const auto& size_dir : size_dirs) {
            for (const auto& ext : extensions) {
                std::string path = base_path + "/" + size_dir + "/" + icon_name + ext;
                if (std::filesystem::exists(path)) {
                    return path;
                }
            }
        }
        
        // Try direct paths (common in /usr/share/pixmaps)
        for (const auto& ext : extensions) {
            std::string path = base_path + "/" + icon_name + ext;
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
    }
    
    return "";
}

cairo_surface_t* IconLoader::LoadIconFromFile(const std::string& filepath, int size) {
    GError* error = nullptr;
    
    // Load pixbuf with automatic scaling
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(
        filepath.c_str(),
        size,
        size,
        &error
    );
    
    if (error) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "Failed to load icon {}: {}", filepath, error->message);
        g_error_free(error);
        return nullptr;
    }
    
    if (!pixbuf) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "Failed to load icon: {}", filepath);
        return nullptr;
    }
    
    // Get pixbuf properties
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    
    // Create Cairo surface
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    unsigned char* data = cairo_image_surface_get_data(surface);
    
    // Copy pixel data from GdkPixbuf to Cairo surface
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int pixbuf_stride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    bool has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar* p = pixels + y * pixbuf_stride + x * n_channels;
            unsigned char* q = data + y * stride + x * 4;
            
            if (has_alpha) {
                // RGBA -> ARGB (Cairo uses premultiplied alpha)
                unsigned char r = p[0];
                unsigned char g = p[1];
                unsigned char b = p[2];
                unsigned char a = p[3];
                
                // Premultiply alpha
                q[0] = (b * a) / 255;  // B
                q[1] = (g * a) / 255;  // G
                q[2] = (r * a) / 255;  // R
                q[3] = a;              // A
            } else {
                // RGB -> ARGB (no alpha)
                q[0] = p[2];  // B
                q[1] = p[1];  // G
                q[2] = p[0];  // R
                q[3] = 255;   // A
            }
        }
    }
    
    cairo_surface_mark_dirty(surface);
    g_object_unref(pixbuf);
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Loaded icon: {} ({}x{})", filepath, width, height);
    return surface;
}

} // namespace UI
} // namespace Leviathan
