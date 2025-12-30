#include "wayland/WallpaperManager.hpp"
#include "wayland/WaylandTypes.hpp"
#include "ui/ShmBuffer.hpp"
#include "config/ConfigParser.hpp"
#include "Logger.hpp"

#include <cairo/cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
// #include <gdk/gdk.h>  // Removed GDK dependency
#include <algorithm>
#include <cstdlib>  // For malloc/free
#include <cstdint>  // For uint32_t

namespace Leviathan {
namespace Wayland {

WallpaperManager::WallpaperManager(struct wlr_scene_tree* background_layer, struct wl_event_loop* event_loop,
                                   int width, int height)
    : background_layer_(background_layer),
      event_loop_(event_loop),
      width_(width),
      height_(height) {
    LOG_DEBUG_FMT("Created WallpaperManager with dimensions: {}x{}", width, height);
}

WallpaperManager::~WallpaperManager() {
    ClearWallpaper();
    LOG_DEBUG("Destroyed WallpaperManager");
}

void WallpaperManager::SetMonitorConfig(const MonitorConfig& config) {
    monitor_config_ = &config;
    
    // Clear any existing wallpaper
    ClearWallpaper();
    
    // Initialize wallpaper if configured
    if (!config.wallpaper.empty()) {
        InitializeWallpaper();
    }
}

void WallpaperManager::ClearWallpaper() {
    // Remove rotation timer
    if (wallpaper_timer_) {
        wl_event_source_remove(wallpaper_timer_);
        wallpaper_timer_ = nullptr;
    }
    
    // Clean up based on wallpaper type
    if (current_type_ == WallpaperType::StaticImage) {
        // Destroy wallpaper scene node (this will also drop the buffer reference)
        if (wallpaper_node_) {
            wlr_scene_node_destroy(wallpaper_node_);
            wallpaper_node_ = nullptr;
        }
        
        // Drop our reference to the buffer if we still have one
        if (wallpaper_buffer_) {
            wallpaper_buffer_ = nullptr;
        }
    }
    // WallpaperEngine support disabled
    /*
    } else if (current_type_ == WallpaperType::WallpaperEngine) {
        // Clean up WallpaperEngine renderer
        if (we_renderer_) {
            we_renderer_->UnloadWallpaper();
            we_renderer_.reset();
        }
    }
    */
    
    // Clear state
    wallpaper_paths_.clear();
    current_wallpaper_path_.clear();
    wallpaper_index_ = 0;
}

void WallpaperManager::NextWallpaper() {
    if (wallpaper_paths_.empty()) {
        return;
    }
    
    // Move to next wallpaper in the list
    wallpaper_index_ = (wallpaper_index_ + 1) % wallpaper_paths_.size();
    const std::string& wallpaper_path = wallpaper_paths_[wallpaper_index_];
    
    LOG_INFO_FMT("Rotating to next wallpaper: {}", wallpaper_path);
    
    if (current_type_ == WallpaperType::StaticImage) {
        // Load the new wallpaper image
        ShmBuffer* new_buffer = LoadWallpaperImage(wallpaper_path, width_, height_);
        if (!new_buffer) {
            LOG_ERROR_FMT("Failed to load wallpaper image '{}' during rotation", wallpaper_path);
            return;
        }
        
        // Update the scene buffer with the new image
        if (wallpaper_node_) {
            struct wlr_scene_buffer* scene_buffer = wlr_scene_buffer_from_node(wallpaper_node_);
            if (scene_buffer) {
                // This will drop the old buffer and lock the new one
                wlr_scene_buffer_set_buffer(scene_buffer, new_buffer->GetWlrBuffer());
                wallpaper_buffer_ = new_buffer;  // Update our pointer
            }
        }
    }
    // WallpaperEngine support disabled
    /*
    } else if (current_type_ == WallpaperType::WallpaperEngine) {
        // Load the new WallpaperEngine wallpaper
        if (we_renderer_) {
            we_renderer_->UnloadWallpaper();
            we_renderer_->LoadWallpaper(wallpaper_path);
        }
    }
    */
    
    current_wallpaper_path_ = wallpaper_path;
}

ShmBuffer* WallpaperManager::LoadWallpaperImage(const std::string& path, int target_width, int target_height) {
    GError* error = nullptr;
    
    // Load the image using gdk-pixbuf (supports PNG, JPEG, BMP, WebP, etc.)
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
    
    if (!pixbuf) {
        LOG_ERROR_FMT("Failed to load wallpaper image '{}': {}", 
                      path, error ? error->message : "unknown error");
        if (error) g_error_free(error);
        return nullptr;
    }
    
    int img_width = gdk_pixbuf_get_width(pixbuf);
    int img_height = gdk_pixbuf_get_height(pixbuf);
    
    LOG_DEBUG_FMT("Loaded wallpaper image: {}x{} from '{}'", img_width, img_height, path);
    
    // Create SHM buffer for the scaled wallpaper
    ShmBuffer* buffer = ShmBuffer::Create(target_width, target_height);
    if (!buffer) {
        LOG_ERROR("Failed to create SHM buffer for wallpaper");
        g_object_unref(pixbuf);
        return nullptr;
    }
    
    // Create Cairo surface for the buffer
    cairo_surface_t* target_surface = cairo_image_surface_create_for_data(
        reinterpret_cast<unsigned char*>(buffer->GetData()),
        CAIRO_FORMAT_ARGB32,
        target_width,
        target_height,
        buffer->GetStride()
    );
    
    cairo_t* cr = cairo_create(target_surface);
    
    // Calculate scaling to cover the entire output (like CSS background-size: cover)
    double scale_x = static_cast<double>(target_width) / img_width;
    double scale_y = static_cast<double>(target_height) / img_height;
    double scale = std::max(scale_x, scale_y);  // Use larger scale to cover
    
    // Center the image
    double scaled_width = img_width * scale;
    double scaled_height = img_height * scale;
    double offset_x = (target_width - scaled_width) / 2.0;
    double offset_y = (target_height - scaled_height) / 2.0;
    
    // Apply transformations and draw
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    
    // Convert GdkPixbuf to Cairo surface manually (replaces gdk_cairo_set_source_pixbuf)
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    int has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
    int pixbuf_stride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);
    
    // Create a new buffer with the correct format for Cairo
    unsigned char* cairo_data = (unsigned char*)malloc(stride * height);
    
    // Convert GdkPixbuf RGB/RGBA to Cairo ARGB32
    for (int y = 0; y < height; y++) {
        guchar* src = pixels + y * pixbuf_stride;
        uint32_t* dst = (uint32_t*)(cairo_data + y * stride);
        
        for (int x = 0; x < width; x++) {
            if (has_alpha) {
                // RGBA -> ARGB32 (premultiply alpha)
                guchar r = src[0];
                guchar g = src[1];
                guchar b = src[2];
                guchar a = src[3];
                dst[x] = (a << 24) | (r << 16) | (g << 8) | b;
                src += 4;
            } else {
                // RGB -> ARGB32 (opaque)
                guchar r = src[0];
                guchar g = src[1];
                guchar b = src[2];
                dst[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                src += 3;
            }
        }
    }
    
    cairo_surface_t* img_surface = cairo_image_surface_create_for_data(
        cairo_data, CAIRO_FORMAT_ARGB32, width, height, stride);
    cairo_set_source_surface(cr, img_surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(img_surface);
    free(cairo_data);
    
    // Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(target_surface);
    g_object_unref(pixbuf);
    
    LOG_INFO_FMT("Scaled wallpaper from {}x{} to {}x{} (scale: {:.2f})", 
                 img_width, img_height, target_width, target_height, scale);
    
    return buffer;
}

void WallpaperManager::InitializeWallpaper() {
    if (!monitor_config_ || monitor_config_->wallpaper.empty()) {
        return;
    }
    
    auto& config = Config();
    
    // Find the wallpaper config by name
    const auto* wallpaper_config = config.wallpapers.FindByName(monitor_config_->wallpaper);
    if (!wallpaper_config) {
        LOG_WARN_FMT("Wallpaper config '{}' not found", monitor_config_->wallpaper);
        return;
    }
    
    if (wallpaper_config->wallpapers.empty()) {
        LOG_WARN_FMT("Wallpaper config '{}' has no wallpaper paths", monitor_config_->wallpaper);
        return;
    }
    
    // Store wallpaper paths for rotation
    wallpaper_paths_ = wallpaper_config->wallpapers;
    wallpaper_index_ = 0;
    current_type_ = wallpaper_config->type;
    
    // Load and render the first wallpaper based on type
    const std::string& wallpaper_path = wallpaper_paths_[wallpaper_index_];
    LOG_INFO_FMT("Setting wallpaper: {} (type: {})", wallpaper_path, 
                 current_type_ == WallpaperType::StaticImage ? "static" : "wallpaper_engine");
    
    if (current_type_ == WallpaperType::StaticImage) {
        // Load static image wallpaper
        InitializeStaticWallpaper(wallpaper_path);
    }
    // WallpaperEngine support disabled
    /*
    } else if (current_type_ == WallpaperType::WallpaperEngine) {
        // Load WallpaperEngine wallpaper
        InitializeWallpaperEngine(wallpaper_path);
    }
    */
    
    current_wallpaper_path_ = wallpaper_path;
    
    // Setup rotation timer if configured
    if (wallpaper_config->change_interval_seconds > 0 && wallpaper_paths_.size() > 1) {
        wallpaper_timer_ = wl_event_loop_add_timer(event_loop_, WallpaperRotationCallback, this);
        if (wallpaper_timer_) {
            wl_event_source_timer_update(wallpaper_timer_, 
                                        wallpaper_config->change_interval_seconds * 1000);
            LOG_INFO_FMT("Started wallpaper rotation every {} seconds",
                        wallpaper_config->change_interval_seconds);
        }
    }
}

void WallpaperManager::InitializeStaticWallpaper(const std::string& wallpaper_path) {
    // Load the image and create buffer
    wallpaper_buffer_ = LoadWallpaperImage(wallpaper_path, width_, height_);
    if (!wallpaper_buffer_) {
        LOG_ERROR_FMT("Failed to load wallpaper image '{}'", wallpaper_path);
        return;
    }
    
    // Create scene buffer node in background layer
    struct wlr_scene_buffer* scene_buffer = wlr_scene_buffer_create(
        background_layer_,
        wallpaper_buffer_->GetWlrBuffer()
    );
    
    if (!scene_buffer) {
        LOG_ERROR("Failed to create scene buffer for wallpaper");
        // Drop our reference to the buffer since we're not using it
        wlr_buffer_drop(wallpaper_buffer_->GetWlrBuffer());
        wallpaper_buffer_ = nullptr;
        return;
    }
    
    wallpaper_node_ = &scene_buffer->node;
    wlr_scene_node_set_position(wallpaper_node_, 0, 0);
}

// WallpaperEngine support disabled
/*
void WallpaperManager::InitializeWallpaperEngine(const std::string& wallpaper_path) {
    // Create WallpaperEngine renderer if not already created
    if (!we_renderer_) {
        // Note: We need to pass the wlroots renderer for texture import
        // This will need to be added to WallpaperManager constructor
        we_renderer_ = std::make_unique<WallpaperEngineRenderer>(
            background_layer_, event_loop_, nullptr, width_, height_
        );
    }
    
    // Load the WallpaperEngine project
    if (!we_renderer_->LoadWallpaper(wallpaper_path)) {
        LOG_ERROR_FMT("Failed to load WallpaperEngine wallpaper '{}'", wallpaper_path);
        we_renderer_.reset();
        return;
    }
    
    LOG_INFO_FMT("Loaded WallpaperEngine wallpaper: {}", wallpaper_path);
}
*/

int WallpaperManager::WallpaperRotationCallback(void* data) {
    WallpaperManager* manager = static_cast<WallpaperManager*>(data);
    manager->NextWallpaper();
    return 0;  // Timer will be re-armed automatically
}

} // namespace Wayland
} // namespace Leviathan
