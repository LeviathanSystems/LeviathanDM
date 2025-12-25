#include "ui/reusable-widgets/Label.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace UI {

void Label::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Create a temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, font_family_.c_str(), 
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
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Save cairo state
    cairo_save(cr);
    
    // Draw background if not transparent
    if (bg_color_[3] > 0.0) {
        cairo_set_source_rgba(cr, bg_color_[0], bg_color_[1], bg_color_[2], bg_color_[3]);
        cairo_rectangle(cr, x_, y_, width_, height_);
        cairo_fill(cr);
    }
    
    // Draw text
    cairo_select_font_face(cr, font_family_.c_str(),
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

} // namespace UI
} // namespace Leviathan
