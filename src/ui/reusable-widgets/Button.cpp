#include "ui/reusable-widgets/Button.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>

namespace Leviathan {
namespace UI {

void Button::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
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
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
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

} // namespace UI
} // namespace Leviathan
