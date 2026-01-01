#include "ui/reusable-widgets/VBox.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace UI {

void VBox::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (children_.empty()) {
        width_ = 0;
        height_ = 0;
        return;
    }
    
    // Calculate total height needed and max width
    int total_height = 0;
    int max_width = 0;
    
    // First pass: calculate each child's preferred size
    // Give children unlimited height to get their natural size
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        child->CalculateSize(available_width, 10000);  // Large height for natural sizing
        total_height += child->GetHeight();
        max_width = std::max(max_width, child->GetWidth());
    }
    
    // Add spacing between children
    int visible_count = 0;
    for (auto& child : children_) {
        if (child->IsVisible()) visible_count++;
    }
    if (visible_count > 1) {
        total_height += spacing_ * (visible_count - 1);
    }
    
    width_ = std::min(max_width, available_width);
    height_ = std::min(total_height, available_height);
    
    // Second pass: position children RELATIVE to this container (starting at 0, 0)
    int current_y = 0;
    
    // Handle alignment within this container's height
    if (align_ == Align::Center && total_height < height_) {
        current_y = (height_ - total_height) / 2;
    } else if (align_ == Align::End && total_height < height_) {
        current_y = height_ - total_height;
    }
    
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        // Horizontal alignment (center children horizontally)
        int child_x = (width_ - child->GetWidth()) / 2;
        
        // Set RELATIVE position (will be translated to absolute during rendering)
        child->SetPosition(child_x, current_y);
        current_y += child->GetHeight() + spacing_;
    }
}

} // namespace UI
} // namespace Leviathan
