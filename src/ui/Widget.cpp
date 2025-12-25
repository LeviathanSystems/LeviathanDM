#include "ui/Widget.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include "ui/reusable-widgets/Button.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace UI {

// ============================================================================
// HBox Implementation
// ============================================================================

void HBox::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (children_.empty()) {
        width_ = 0;
        height_ = 0;
        return;
    }
    
    // Calculate total width needed and max height
    int total_width = 0;
    int max_height = 0;
    
    // First pass: calculate each child's preferred size
    // Give children unlimited width to get their natural size
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        child->CalculateSize(10000, available_height);  // Large width for natural sizing
        total_width += child->GetWidth();
        max_height = std::max(max_height, child->GetHeight());
    }
    
    // Add spacing between children (except for Apart alignment which handles spacing differently)
    int visible_count = 0;
    for (auto& child : children_) {
        if (child->IsVisible()) visible_count++;
    }
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
        
        // Position first child at start
        int child_y = (height_ - visible_children[0]->GetHeight()) / 2;
        visible_children[0]->SetPosition(x_, y_ + child_y);
        
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
            visible_children[i]->SetPosition(x_ + middle_x, y_ + child_y);
            middle_x += visible_children[i]->GetWidth() + spacing_;
        }
        
        // Position last child at end
        int last_idx = visible_children.size() - 1;
        child_y = (height_ - visible_children[last_idx]->GetHeight()) / 2;
        int last_x = width_ - visible_children[last_idx]->GetWidth();
        visible_children[last_idx]->SetPosition(x_ + last_x, y_ + child_y);
        
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
        
        // Set position relative to container, then add container's position
        child->SetPosition(x_ + current_x, y_ + child_y);
        current_x += child->GetWidth() + spacing_;
    }
}

// ============================================================================
// VBox Implementation
// ============================================================================

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
    
    // Second pass: position children RELATIVE to this container
    int current_y = 0;  // Start at 0, relative to container
    
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
        
        // Set position relative to container, then add container's position
        child->SetPosition(x_ + child_x, y_ + current_y);
        current_y += child->GetHeight() + spacing_;
    }
}

} // namespace UI
} // namespace Leviathan
