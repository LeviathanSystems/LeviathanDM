#include "wayland/LayerManager.hpp"
#include "wayland/WaylandTypes.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace Wayland {

LayerManager::LayerManager(struct wlr_scene* scene) {
    // Create layer trees in order (bottom to top)
    layers_[static_cast<size_t>(Layer::Background)] = 
        wlr_scene_tree_create(&scene->tree);
    
    layers_[static_cast<size_t>(Layer::WorkingArea)] = 
        wlr_scene_tree_create(&scene->tree);
    
    layers_[static_cast<size_t>(Layer::Top)] = 
        wlr_scene_tree_create(&scene->tree);
    
    LOG_INFO("Created layer hierarchy:");
    LOG_INFO("  - Background layer: {}", static_cast<void*>(layers_[0]));
    LOG_INFO("  - Working area layer: {}", static_cast<void*>(layers_[1]));
    LOG_INFO("  - Top layer: {}", static_cast<void*>(layers_[2]));
}

struct wlr_scene_tree* LayerManager::GetLayer(Layer layer) {
    size_t index = static_cast<size_t>(layer);
    if (index >= static_cast<size_t>(Layer::COUNT)) {
        LOG_ERROR("Invalid layer index: {}", index);
        return nullptr;
    }
    return layers_[index];
}

void LayerManager::SetReservedSpace(const ReservedSpace& space) {
    reserved_space_ = space;
    LOG_INFO("Reserved space updated: top={}, bottom={}, left={}, right={}",
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
    
    LOG_DEBUG("Usable area: x={}, y={}, w={}, h={}", 
              area.x, area.y, area.width, area.height);
    
    return area;
}

} // namespace Wayland
} // namespace Leviathan
