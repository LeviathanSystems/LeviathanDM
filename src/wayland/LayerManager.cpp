#include "wayland/LayerManager.hpp"
#include "wayland/WaylandTypes.hpp"
#include "wayland/View.hpp"
#include "core/Tag.hpp"
#include "core/Client.hpp"
#include "core/Events.hpp"
#include "layout/TilingLayout.hpp"
#include "config/ConfigParser.hpp"
#include "ui/StatusBar.hpp"
#include "ui/ShmBuffer.hpp"
#include "Logger.hpp"
#include "Types.hpp"  // For LayoutType

#include <wlr/types/wlr_scene.h>
#include <cairo/cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
// #include <gdk/gdk.h>  // Removed GDK dependency
#include <algorithm>
#include <cstring>

namespace Leviathan {
namespace Wayland {

LayerManager::LayerManager(struct wlr_scene* scene, struct wlr_output* output, struct wl_event_loop* event_loop)
    : output_(output),
      event_loop_(event_loop) {
    // Create layer trees in order (bottom to top)
    layers_[static_cast<size_t>(Layer::Background)] = 
        wlr_scene_tree_create(&scene->tree);
    
    layers_[static_cast<size_t>(Layer::WorkingArea)] = 
        wlr_scene_tree_create(&scene->tree);
    
    layers_[static_cast<size_t>(Layer::Top)] = 
        wlr_scene_tree_create(&scene->tree);
    
    // Ensure proper stacking order - raise each layer in sequence
    // This ensures Top layer is always above WorkingArea, which is above Background
    wlr_scene_node_raise_to_top(&layers_[static_cast<size_t>(Layer::Background)]->node);
    wlr_scene_node_raise_to_top(&layers_[static_cast<size_t>(Layer::WorkingArea)]->node);
    wlr_scene_node_raise_to_top(&layers_[static_cast<size_t>(Layer::Top)]->node);
    
    // Create layout engine for this output
    layout_engine_ = new TilingLayout();
    
    // Wallpaper will be initialized when SetMonitorConfig is called
    
    //LOG_INFO_FMT("Created per-output layer hierarchy for '{}':", output->name);
    //LOG_INFO_FMT("  - Background layer: {}", static_cast<void*>(layers_[0]));
    //LOG_INFO_FMT("  - Working area layer: {}", static_cast<void*>(layers_[1]));
    //LOG_INFO_FMT("  - Top layer: {}", static_cast<void*>(layers_[2]));
}

LayerManager::~LayerManager() {
    // Clean up wallpaper
    ClearWallpaper();
    
    // Clean up layout engine
    delete layout_engine_;
    
    // Scene trees are cleaned up automatically by wlroots
    LOG_DEBUG_FMT("Destroyed LayerManager for output '{}'", output_ ? output_->name : "unknown");
}

void LayerManager::SetMonitorConfig(const MonitorConfig& config) {
    monitor_config_ = &config;
    
    // Initialize wallpaper if configured
    if (!config.wallpaper.empty()) {
        InitializeWallpaper();
    }
}

struct wlr_scene_tree* LayerManager::GetLayer(Layer layer) {
    size_t index = static_cast<size_t>(layer);
    if (index >= static_cast<size_t>(Layer::COUNT)) {
        LOG_ERROR_FMT("Invalid layer index: {}", index);
        return nullptr;
    }
    return layers_[index];
}

void LayerManager::SetReservedSpace(const ReservedSpace& space) {
    reserved_space_ = space;
    LOG_INFO_FMT("Reserved space updated: top={}, bottom={}, left={}, right={}",
             space.top, space.bottom, space.left, space.right);
}

UsableArea LayerManager::CalculateUsableArea(int32_t output_x, int32_t output_y,
                                             uint32_t output_width, uint32_t output_height) const {
    UsableArea area;
    
    // Start with full output area
    area.x = output_x + static_cast<int32_t>(reserved_space_.left);
    area.y = output_y + static_cast<int32_t>(reserved_space_.top);
    
    // Subtract reserved space from width and height
    area.width = output_width - reserved_space_.left - reserved_space_.right;
    area.height = output_height - reserved_space_.top - reserved_space_.bottom;
    
    LOG_DEBUG_FMT("Usable area: x={}, y={}, w={}, h={}", 
              area.x, area.y, area.width, area.height);
    
    return area;
}

void LayerManager::TileViews(std::vector<View*>& views,
                             Core::Tag* tag,
                             TilingLayout* layout_engine) {
    if (!output_ || !tag || !layout_engine) {
        LOG_WARN_FMT("TileViews: Invalid parameters (output={}, tag={}, layout={})",
                 static_cast<void*>(output_), static_cast<void*>(tag), 
                 static_cast<void*>(layout_engine));
        return;
    }
    
    if (views.empty()) {
        LOG_DEBUG_FMT("TileViews: No views to tile on output '{}'", output_->name);
        return;
    }
    
    // Calculate usable workspace area for this output
    auto workspace = CalculateUsableArea(0, 0, output_->width, output_->height);
    
    LOG_DEBUG_FMT("Tiling {} views on output '{}' in workspace: pos=({},{}), size=({}x{})",
              views.size(), output_->name,
              workspace.x, workspace.y, workspace.width, workspace.height);
    
    int gap = Config().general.gap_size;
    int master_count = tag->GetMasterCount();
    float master_ratio = tag->GetMasterRatio();
    
    // Apply layout algorithm
    // The layout engine will calculate positions relative to workspace (0,0)
    // and set both view positions and scene node positions
    switch (tag->GetLayout()) {
        case LayoutType::MASTER_STACK:
            layout_engine->ApplyMasterStack(views, master_count, master_ratio,
                                           workspace.width, workspace.height, gap);
            break;
        case LayoutType::MONOCLE:
            layout_engine->ApplyMonocle(views, workspace.width, workspace.height);
            break;
        case LayoutType::GRID:
            layout_engine->ApplyGrid(views, workspace.width, workspace.height, gap);
            break;
        default:
            LOG_WARN("Unknown layout type");
            break;
    }
    
    // If workspace has an offset (e.g., reserved space), adjust all scene positions
    if (workspace.x != 0 || workspace.y != 0) {
        for (auto* view : views) {
            if (view && view->scene_tree) {
                // Layout engine set position relative to (0,0)
                // We need to offset by workspace position
                wlr_scene_node_set_position(&view->scene_tree->node,
                                           view->x + workspace.x,
                                           view->y + workspace.y);
                LOG_DEBUG_FMT("  Offset view to ({},{}) for workspace at ({},{})",
                         view->x + workspace.x, view->y + workspace.y,
                         workspace.x, workspace.y);
            }
        }
    }
}

void LayerManager::AddStatusBar(Leviathan::StatusBar* bar) {
    if (!bar) {
        LOG_WARN("Attempted to add null StatusBar to LayerManager");
        return;
    }
    
    status_bars_.push_back(bar);
    LOG_DEBUG_FMT("Added StatusBar to LayerManager for output '{}'", output_->name);
}

void LayerManager::RemoveStatusBar(Leviathan::StatusBar* bar) {
    auto it = std::find(status_bars_.begin(), status_bars_.end(), bar);
    if (it != status_bars_.end()) {
        status_bars_.erase(it);
        LOG_DEBUG_FMT("Removed StatusBar from LayerManager for output '{}'", output_->name);
    }
}

void LayerManager::CreateStatusBars(const std::vector<std::string>& bar_names,
                                   const StatusBarsConfig& all_bars_config,
                                   uint32_t output_width,
                                   uint32_t output_height) {
    if (bar_names.empty()) {
        LOG_DEBUG_FMT("No status bars to create for output '{}'", output_->name);
        return;
    }
    
    LOG_INFO_FMT("Creating {} status bar(s) for output '{}'", 
             bar_names.size(), output_->name);
    
    ReservedSpace reserved = reserved_space_;
    
    for (const auto& bar_name : bar_names) {
        const StatusBarConfig* bar_config = all_bars_config.FindByName(bar_name);
        
        if (!bar_config) {
            LOG_WARN_FMT("Status bar '{}' not found in configuration", bar_name);
            continue;
        }
        
        // Reserve space for this status bar
        switch (bar_config->position) {
            case StatusBarConfig::Position::Top:
                reserved.top += bar_config->height;
                LOG_INFO_FMT("Reserved {}px at top for status bar '{}'", 
                         bar_config->height, bar_name);
                break;
            
            case StatusBarConfig::Position::Bottom:
                reserved.bottom += bar_config->height;
                LOG_INFO_FMT("Reserved {}px at bottom for status bar '{}'", 
                         bar_config->height, bar_name);
                break;
            
            case StatusBarConfig::Position::Left:
                reserved.left += bar_config->width;
                LOG_INFO_FMT("Reserved {}px at left for status bar '{}'", 
                         bar_config->width, bar_name);
                break;
            
            case StatusBarConfig::Position::Right:
                reserved.right += bar_config->width;
                LOG_INFO_FMT("Reserved {}px at right for status bar '{}'", 
                         bar_config->width, bar_name);
                break;
        }
        
        // Create and render the StatusBar
        Leviathan::StatusBar* bar = new Leviathan::StatusBar(*bar_config, this, event_loop_, output_width, output_height);
        AddStatusBar(bar);
    }
    
    // Apply the accumulated reserved space
    SetReservedSpace(reserved);
    LOG_DEBUG_FMT("Total reserved space: top={}, bottom={}, left={}, right={}", 
             reserved.top, reserved.bottom, reserved.left, reserved.right);
}

// Tag management implementation
void LayerManager::InitializeTags(const std::vector<TagConfig>& tag_configs) {
    tags_.clear();
    current_tag_index_ = 0;
    
    if (tag_configs.empty()) {
        LOG_WARN_FMT("No tag configs provided to LayerManager for output '{}'", output_->name);
        // Create default single tag
        auto tag = std::make_unique<Core::Tag>("1");
        tag->SetVisible(true);
        tags_.push_back(std::move(tag));
    } else {
        // Create tags from config
        for (const auto& tag_config : tag_configs) {
            std::string tag_name = tag_config.name;
            if (!tag_config.icon.empty()) {
                tag_name = tag_config.icon + " " + tag_config.name;
            }
            auto tag = std::make_unique<Core::Tag>(tag_name);
            // First tag is visible by default
            tag->SetVisible(tags_.empty());
            tags_.push_back(std::move(tag));
            LOG_DEBUG_FMT("Created tag {} for output '{}': {}", tag_config.id, output_->name, tag_name);
        }
    }
    
    LOG_INFO_FMT("Initialized {} tags for output '{}'", tags_.size(), output_->name);
}

void LayerManager::SwitchToTag(int index) {
    if (index < 0 || index >= static_cast<int>(tags_.size())) {
        LOG_WARN_FMT("Invalid tag index {} for output '{}' (has {} tags)", 
                 index, output_->name, tags_.size());
        return;
    }
    
    if (index == current_tag_index_) {
        LOG_DEBUG_FMT("Already on tag {} for output '{}'", index, output_->name);
        return;
    }
    
    // Get old and new tags for event
    Core::Tag* old_tag = (current_tag_index_ >= 0 && current_tag_index_ < static_cast<int>(tags_.size())) 
                         ? tags_[current_tag_index_].get() : nullptr;
    Core::Tag* new_tag = tags_[index].get();
    
    // Hide current tag
    if (old_tag) {
        old_tag->SetVisible(false);
        LOG_DEBUG_FMT("Hiding tag {} on output '{}'", current_tag_index_, output_->name);
    }
    
    // Switch to new tag
    current_tag_index_ = index;
    new_tag->SetVisible(true);
    LOG_INFO_FMT("Switched to tag {} on output '{}'", index, output_->name);
    
    // Publish tag switched event
    Core::TagSwitchedEvent event(old_tag, new_tag, nullptr);  // TODO: Pass screen once available
    Core::EventBus::Instance().Publish(event);
    LOG_DEBUG_FMT("Published TagSwitchedEvent for output '{}'", output_->name);
}

Core::Tag* LayerManager::GetCurrentTag() {
    if (current_tag_index_ >= 0 && current_tag_index_ < static_cast<int>(tags_.size())) {
        return tags_[current_tag_index_].get();
    }
    return nullptr;
}

std::vector<Core::Tag*> LayerManager::GetTags() const {
    std::vector<Core::Tag*> result;
    result.reserve(tags_.size());
    for (const auto& tag : tags_) {
        result.push_back(tag.get());
    }
    return result;
}

void LayerManager::SetLayout(LayoutType layout) {
    auto* tag = GetCurrentTag();
    if (!tag) {
        LOG_WARN("SetLayout: No active tag");
        return;
    }
    
    tag->SetLayout(layout);
    AutoTile();
    
    LOG_DEBUG_FMT("Set layout for output '{}' tag {}", output_->name, current_tag_index_);
}

void LayerManager::IncreaseMasterCount() {
    auto* tag = GetCurrentTag();
    if (!tag) {
        return;
    }
    
    int current = tag->GetMasterCount();
    tag->SetMasterCount(current + 1);
    AutoTile();
}

void LayerManager::DecreaseMasterCount() {
    auto* tag = GetCurrentTag();
    if (!tag) {
        return;
    }
    
    int current = tag->GetMasterCount();
    if (current > 1) {
        tag->SetMasterCount(current - 1);
        AutoTile();
    }
}

void LayerManager::IncreaseMasterRatio() {
    auto* tag = GetCurrentTag();
    if (!tag) {
        return;
    }
    
    float current = tag->GetMasterRatio();
    if (current < 0.95f) {
        tag->SetMasterRatio(current + 0.05f);
        AutoTile();
    }
}

void LayerManager::DecreaseMasterRatio() {
    auto* tag = GetCurrentTag();
    if (!tag) {
        return;
    }
    
    float current = tag->GetMasterRatio();
    if (current > 0.05f) {
        tag->SetMasterRatio(current - 0.05f);
        AutoTile();
    }
}

void LayerManager::MoveClientToTag(Core::Client* client, int target_tag_index) {
    if (!client) {
        return;
    }
    
    if (target_tag_index < 0 || target_tag_index >= static_cast<int>(tags_.size())) {
        LOG_WARN_FMT("MoveClientToTag: Invalid tag index {} (have {} tags)", 
                   target_tag_index, tags_.size());
        return;
    }
    
    auto* current_tag = GetCurrentTag();
    auto* target_tag = tags_[target_tag_index].get();
    
    if (current_tag == target_tag) {
        return;  // Already on this tag
    }
    
    // Remove from current tag
    if (current_tag) {
        current_tag->RemoveClient(client);
    }
    
    // Add to target tag
    target_tag->AddClient(client);
    
    // Hide the client's view since it's moving to an inactive tag
    auto* view = client->GetView();
    if (view && view->scene_tree) {
        wlr_scene_node_set_enabled(&view->scene_tree->node, target_tag_index == current_tag_index_);
    }
    
    // Re-tile current tag
    AutoTile();
    
    LOG_DEBUG_FMT("Moved client to tag {} on output '{}'", target_tag_index, output_->name);
}

void LayerManager::AddView(View* view) {
    if (!view) {
        return;
    }
    
    LOG_DEBUG_FMT("AddView called for view {} on output '{}'", 
                  static_cast<void*>(view), output_->name);
    
    // View will be tiled when it's mapped
    // We'll trigger auto-tiling in the map handler
}

void LayerManager::RemoveView(View* view) {
    if (!view) {
        return;
    }
    
    LOG_DEBUG_FMT("RemoveView called for view {} on output '{}'", 
                  static_cast<void*>(view), output_->name);
    
    // Trigger auto-tiling to reorganize remaining views
    AutoTile();
}

void LayerManager::AutoTile() {
    auto* tag = GetCurrentTag();
    if (!tag || !layout_engine_) {
        return;
    }
    
    // Collect views that should be tiled
    std::vector<View*> tiled_views;
    const auto& clients = tag->GetClients();
    for (auto* client : clients) {
        auto* view = client->GetView();
        if (view && view->mapped && !view->is_floating && !view->is_fullscreen) {
            tiled_views.push_back(view);
        }
    }
    
    if (!tiled_views.empty()) {
        TileViews(tiled_views, tag, layout_engine_);
        LOG_DEBUG_FMT("Auto-tiled {} views on output '{}'", tiled_views.size(), output_->name);
    }
}

// Helper function to load and scale wallpaper image
ShmBuffer* LayerManager::LoadWallpaperImage(const std::string& path, int target_width, int target_height) {
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

void LayerManager::InitializeWallpaper() {
    if (!monitor_config_ || monitor_config_->wallpaper.empty()) {
        return;
    }
    
    auto& config = Config();
    
    // Find the wallpaper config by name
    const auto* wallpaper_config = config.wallpapers.FindByName(monitor_config_->wallpaper);
    if (!wallpaper_config) {
        LOG_WARN_FMT("Wallpaper config '{}' not found for output '{}'", 
                     monitor_config_->wallpaper, output_->name);
        return;
    }
    
    if (wallpaper_config->wallpapers.empty()) {
        LOG_WARN_FMT("Wallpaper config '{}' has no wallpaper paths", monitor_config_->wallpaper);
        return;
    }
    
    // Store wallpaper paths for rotation
    wallpaper_paths_ = wallpaper_config->wallpapers;
    wallpaper_index_ = 0;
    
    // Load and render the first wallpaper
    const std::string& wallpaper_path = wallpaper_paths_[wallpaper_index_];
    LOG_INFO_FMT("Setting wallpaper for output '{}': {}", output_->name, wallpaper_path);
    
    // Load the image and create buffer
    wallpaper_buffer_ = LoadWallpaperImage(wallpaper_path, output_->width, output_->height);
    if (!wallpaper_buffer_) {
        LOG_ERROR_FMT("Failed to load wallpaper image '{}'", wallpaper_path);
        return;
    }
    
    // Create scene buffer node in background layer
    struct wlr_scene_buffer* scene_buffer = wlr_scene_buffer_create(
        GetBackgroundLayer(),
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

    
    current_wallpaper_path_ = wallpaper_path;
    
    // Setup rotation timer if configured
    if (wallpaper_config->change_interval_seconds > 0 && wallpaper_paths_.size() > 1) {
        wallpaper_timer_ = wl_event_loop_add_timer(event_loop_, WallpaperRotationCallback, this);
        if (wallpaper_timer_) {
            wl_event_source_timer_update(wallpaper_timer_, 
                                        wallpaper_config->change_interval_seconds * 1000);
            LOG_INFO_FMT("Started wallpaper rotation every {} seconds for output '{}'",
                        wallpaper_config->change_interval_seconds, output_->name);
        }
    }
}

void LayerManager::ClearWallpaper() {
    // Remove rotation timer
    if (wallpaper_timer_) {
        wl_event_source_remove(wallpaper_timer_);
        wallpaper_timer_ = nullptr;
    }
    
    // Destroy wallpaper scene node (this will also drop the buffer reference)
    if (wallpaper_node_) {
        wlr_scene_node_destroy(wallpaper_node_);
        wallpaper_node_ = nullptr;
    }
    
    // Drop our reference to the buffer if we still have one
    // (normally the scene node destruction handles this)
    if (wallpaper_buffer_) {
        // The buffer is reference counted and will be freed when refcount reaches 0
        wallpaper_buffer_ = nullptr;
    }
    
    // Clear state
    wallpaper_paths_.clear();
    current_wallpaper_path_.clear();
    wallpaper_index_ = 0;
}

void LayerManager::NextWallpaper() {
    if (wallpaper_paths_.empty()) {
        return;
    }
    
    // Move to next wallpaper in the list
    wallpaper_index_ = (wallpaper_index_ + 1) % wallpaper_paths_.size();
    const std::string& wallpaper_path = wallpaper_paths_[wallpaper_index_];
    
    LOG_INFO_FMT("Rotating to next wallpaper for output '{}': {}", output_->name, wallpaper_path);
    
    // Note: We don't drop the old buffer reference here because the scene
    // buffer node holds it. When we call wlr_scene_buffer_set_buffer, it will
    // drop the old buffer and take a reference to the new one.
    
    // Load the new wallpaper image
    ShmBuffer* new_buffer = LoadWallpaperImage(wallpaper_path, output_->width, output_->height);
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
    
    current_wallpaper_path_ = wallpaper_path;
}

int LayerManager::WallpaperRotationCallback(void* data) {
    LayerManager* manager = static_cast<LayerManager*>(data);
    manager->NextWallpaper();
    return 0;  // Timer will be re-armed in SetWallpaper if needed
}

} // namespace Wayland
} // namespace Leviathan

