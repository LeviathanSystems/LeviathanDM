#include "wayland/LayerManager.hpp"
#include "wayland/WaylandTypes.hpp"
#include "wayland/View.hpp"
#include "core/Tag.hpp"
#include "core/Client.hpp"
#include "core/Events.hpp"
#include "layout/TilingLayout.hpp"
#include "config/ConfigParser.hpp"
#include "ui/StatusBar.hpp"
#include "Logger.hpp"
#include "Types.hpp"  // For LayoutType

#include <wlr/types/wlr_scene.h>
#include <algorithm>

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
    
    //LOG_INFO_FMT("Created per-output layer hierarchy for '{}':", output->name);
    //LOG_INFO_FMT("  - Background layer: {}", static_cast<void*>(layers_[0]));
    //LOG_INFO_FMT("  - Working area layer: {}", static_cast<void*>(layers_[1]));
    //LOG_INFO_FMT("  - Top layer: {}", static_cast<void*>(layers_[2]));
}

LayerManager::~LayerManager() {
    // Clean up layout engine
    delete layout_engine_;
    
    // Scene trees are cleaned up automatically by wlroots
    LOG_DEBUG_FMT("Destroyed LayerManager for output '{}'", output_ ? output_->name : "unknown");
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

} // namespace Wayland
} // namespace Leviathan

