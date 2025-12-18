#include "ui/Widget.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace UI {

// ============================================================================
// Label Implementation
// ============================================================================

void Label::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create a temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, font_size_);
    
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, text_.c_str(), &extents);
    
    // Set size based on text with padding
    width_ = static_cast<int>(extents.width) + (padding_ * 2);
    height_ = static_cast<int>(extents.height) + (padding_ * 2);
    
    // Constrain to available space
    width_ = std::min(width_, available_width);
    height_ = std::min(height_, available_height);
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
}

void Label::Render(cairo_t* cr) {
    if (!IsVisible()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Save cairo state
    cairo_save(cr);
    
    // Draw background if not transparent
    if (bg_color_[3] > 0.0) {
        cairo_set_source_rgba(cr, bg_color_[0], bg_color_[1], bg_color_[2], bg_color_[3]);
        cairo_rectangle(cr, x_, y_, width_, height_);
        cairo_fill(cr);
    }
    
    // Draw text
    cairo_select_font_face(cr, "sans-serif",
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size_);
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text_.c_str(), &extents);
    
    // Center text vertically and horizontally within the label
    double text_x = x_ + padding_;
    double text_y = y_ + (height_ / 2.0) + (extents.height / 2.0);
    
    cairo_set_source_rgba(cr, text_color_[0], text_color_[1], text_color_[2], text_color_[3]);
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, text_.c_str());
    
    // Restore cairo state
    cairo_restore(cr);
}

// ============================================================================
// Button Implementation
// ============================================================================

void Button::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create a temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, "sans-serif",
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(temp_cr, font_size_);
    
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, text_.c_str(), &extents);
    
    // Set size based on text with padding
    width_ = static_cast<int>(extents.width) + (padding_ * 2);
    height_ = static_cast<int>(extents.height) + (padding_ * 2);
    
    // Constrain to available space
    width_ = std::min(width_, available_width);
    height_ = std::min(height_, available_height);
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
}

void Button::Render(cairo_t* cr) {
    if (!IsVisible()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Save cairo state
    cairo_save(cr);
    
    // Draw background (use hover color if hovered)
    const double* bg = hovered_ ? hover_color_ : bg_color_;
    cairo_set_source_rgba(cr, bg[0], bg[1], bg[2], bg[3]);
    
    // Rounded rectangle
    double radius = 4.0;
    double x = x_;
    double y = y_;
    double w = width_;
    double h = height_;
    
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius, radius, -M_PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI/2);
    cairo_arc(cr, x + radius, y + h - radius, radius, M_PI/2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    // Draw text
    cairo_select_font_face(cr, "sans-serif",
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_size_);
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text_.c_str(), &extents);
    
    // Center text
    double text_x = x + (width_ / 2.0) - (extents.width / 2.0);
    double text_y = y + (height_ / 2.0) + (extents.height / 2.0);
    
    cairo_set_source_rgba(cr, text_color_[0], text_color_[1], text_color_[2], text_color_[3]);
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, text_.c_str());
    
    // Restore cairo state
    cairo_restore(cr);
}

// ============================================================================
// HBox Implementation
// ============================================================================

void HBox::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (children_.empty()) {
        width_ = 0;
        height_ = 0;
        return;
    }
    
    // Calculate total width needed and max height
    int total_width = 0;
    int max_height = 0;
    
    // First pass: calculate each child's preferred size
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        child->CalculateSize(available_width, available_height);
        total_width += child->GetWidth();
        max_height = std::max(max_height, child->GetHeight());
    }
    
    // Add spacing between children
    int visible_count = 0;
    for (auto& child : children_) {
        if (child->IsVisible()) visible_count++;
    }
    if (visible_count > 1) {
        total_width += spacing_ * (visible_count - 1);
    }
    
    width_ = std::min(total_width, available_width);
    height_ = std::min(max_height, available_height);
    
    // Second pass: position children
    int current_x = x_;
    
    // Handle alignment
    if (align_ == Align::Center && total_width < available_width) {
        current_x += (available_width - total_width) / 2;
    } else if (align_ == Align::End && total_width < available_width) {
        current_x += (available_width - total_width);
    }
    
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        // Vertical alignment (center children vertically)
        int child_y = y_ + (height_ - child->GetHeight()) / 2;
        
        child->SetPosition(current_x, child_y);
        current_x += child->GetWidth() + spacing_;
    }
}

// ============================================================================
// VBox Implementation
// ============================================================================

void VBox::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (children_.empty()) {
        width_ = 0;
        height_ = 0;
        return;
    }
    
    // Calculate total height needed and max width
    int total_height = 0;
    int max_width = 0;
    
    // First pass: calculate each child's preferred size
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        child->CalculateSize(available_width, available_height);
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
    
    // Second pass: position children
    int current_y = y_;
    
    // Handle alignment
    if (align_ == Align::Center && total_height < available_height) {
        current_y += (available_height - total_height) / 2;
    } else if (align_ == Align::End && total_height < available_height) {
        current_y += (available_height - total_height);
    }
    
    for (auto& child : children_) {
        if (!child->IsVisible()) continue;
        
        // Horizontal alignment (center children horizontally)
        int child_x = x_ + (width_ - child->GetWidth()) / 2;
        
        child->SetPosition(child_x, current_y);
        current_y += child->GetHeight() + spacing_;
    }
}

} // namespace UI
} // namespace Leviathan
