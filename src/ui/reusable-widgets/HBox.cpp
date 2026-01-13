#include "ui/reusable-widgets/HBox.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace UI {

void HBox::CalculateSize(int available_width, int available_height) {
    
    if (children_.empty()) {
        width_ = 0;
        height_ = 0;
        return;
    }
    
    // Calculate total width needed and max height
    int total_width = 0;
    int max_height = 0;
    
    // Count visible children
    int visible_count = 0;
    for (auto& child : children_) {
        if (child->IsVisible()) visible_count++;
    }
    
    // First pass: Let each child calculate its INTRINSIC/NATURAL size
    // Pass the full available width so children can determine their own needs
    // Children should return their minimum/natural size, not necessarily use all available space
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        // Let child calculate its natural size (it should use only what it needs)
        child->CalculateSize(available_width, available_height);
        total_width += child->GetWidth();
        max_height = std::max(max_height, child->GetHeight());
    }
    
    // Add spacing between children (except for Apart alignment which handles spacing differently)
    if (visible_count > 1 && align_ != Align::Apart) {
        total_width += spacing_ * (visible_count - 1);
    }
    
    // For Apart alignment, use full available width to distribute children
    // For other alignments, use natural size constrained by available width
    if (align_ == Align::Apart) {
        width_ = available_width;  // Use full width for distribution
    } else {
        width_ = std::min(total_width, available_width);
    }
    height_ = std::min(max_height, available_height);
    
    // Second pass: position children RELATIVE to this container
    int current_x = 0;  // Start at 0, relative to container
    
    // Handle special "Apart" alignment: distribute first/middle/last
    if (align_ == Align::Apart && visible_count >= 3) {
        // Get visible children
        std::vector<std::shared_ptr<Widget>> visible_children;
        for (auto& child : children_) {
            if (child->IsVisible()) visible_children.push_back(child);
        }
        
        // Position first child at start (relative to container)
        int child_y = (height_ - visible_children[0]->GetHeight()) / 2;
        visible_children[0]->SetPosition(0, child_y);
        
        // Position middle children in center
        int middle_start = 1;
        int middle_end = visible_children.size() - 1;
        int middle_width = 0;
        for (int i = middle_start; i < middle_end; i++) {
            middle_width += visible_children[i]->GetWidth();
            if (i > middle_start) middle_width += spacing_;
        }
        
        int middle_x = (width_ - middle_width) / 2;
        for (int i = middle_start; i < middle_end; i++) {
            child_y = (height_ - visible_children[i]->GetHeight()) / 2;
            visible_children[i]->SetPosition(middle_x, child_y);
            middle_x += visible_children[i]->GetWidth() + spacing_;
        }
        
        // Position last child at end
        int last_idx = visible_children.size() - 1;
        child_y = (height_ - visible_children[last_idx]->GetHeight()) / 2;
        int last_x = width_ - visible_children[last_idx]->GetWidth();
        visible_children[last_idx]->SetPosition(last_x, child_y);
        
        return;
    }
    
    // Handle standard alignment
    if (align_ == Align::Center && total_width < width_) {
        current_x = (width_ - total_width) / 2;
    } else if (align_ == Align::End && total_width < width_) {
        current_x = width_ - total_width;
    }
    
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        // Vertical alignment (center children vertically)
        int child_y = (height_ - child->GetHeight()) / 2;
        
        // Set RELATIVE position (will be translated to absolute during rendering)
        child->SetPosition(current_x, child_y);
        current_x += child->GetWidth() + spacing_;
    }
}

} // namespace UI
} // namespace Leviathan
