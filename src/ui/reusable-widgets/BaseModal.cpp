#include "ui/reusable-widgets/BaseModal.hpp"
#include "ui/BaseWidget.hpp"
#include <cmath>

namespace Leviathan {
namespace UI {

Modal::Modal() 
    : visible_(false)
    , title_("")
    , screen_width_(0)
    , screen_height_(0)
    , content_width_(600)
    , content_height_(400)
    , padding_(20)
    , corner_radius_(12)
    , font_size_(14)
    , title_font_size_(18)
    , border_width_(1)
{
    // Default dark theme colors
    SetOverlayColor(0.0, 0.0, 0.0, 0.7);      // Semi-transparent black
    SetBackgroundColor(0.15, 0.15, 0.15, 1.0); // Dark gray
    SetTextColor(1.0, 1.0, 1.0, 1.0);          // White
    SetBorderColor(0.3, 0.3, 0.3, 1.0);        // Light gray
}

void Modal::Render(cairo_t* cr) {
    if (!visible_) return;
    
    // Draw full-screen overlay
    cairo_set_source_rgba(cr, overlay_color_[0], overlay_color_[1], 
                         overlay_color_[2], overlay_color_[3]);
    cairo_rectangle(cr, 0, 0, screen_width_, screen_height_);
    cairo_fill(cr);
    
    // Calculate centered position
    int content_x = (screen_width_ - content_width_) / 2;
    int content_y = (screen_height_ - content_height_) / 2;
    
    // Draw modal background with rounded corners
    DrawRoundedRectangle(cr, content_x, content_y, 
                        content_width_, content_height_, corner_radius_);
    cairo_set_source_rgba(cr, background_color_[0], background_color_[1],
                         background_color_[2], background_color_[3]);
    cairo_fill_preserve(cr);
    
    // Draw border
    cairo_set_source_rgba(cr, border_color_[0], border_color_[1],
                         border_color_[2], border_color_[3]);
    cairo_set_line_width(cr, border_width_);
    cairo_stroke(cr);
    
    // Draw title bar if title is set
    int title_height = 0;
    if (!title_.empty()) {
        title_height = title_font_size_ + padding_ * 2;
        
        // Title background (slightly lighter)
        DrawRoundedRectangle(cr, content_x, content_y, 
                            content_width_, title_height, corner_radius_);
        cairo_set_source_rgba(cr, background_color_[0] + 0.05, 
                             background_color_[1] + 0.05,
                             background_color_[2] + 0.05, 1.0);
        cairo_fill(cr);
        
        // Title text
        cairo_select_font_face(cr, "sans-serif", 
                              CAIRO_FONT_SLANT_NORMAL, 
                              CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, title_font_size_);
        cairo_set_source_rgba(cr, text_color_[0], text_color_[1],
                             text_color_[2], text_color_[3]);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, title_.c_str(), &extents);
        
        int text_x = content_x + (content_width_ - extents.width) / 2;
        int text_y = content_y + padding_ + title_font_size_;
        
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, title_.c_str());
        
        // Draw separator line
        cairo_set_source_rgba(cr, border_color_[0], border_color_[1],
                             border_color_[2], border_color_[3]);
        cairo_move_to(cr, content_x + padding_, content_y + title_height);
        cairo_line_to(cr, content_x + content_width_ - padding_, 
                     content_y + title_height);
        cairo_stroke(cr);
    }
    
    // Render content (implemented by subclasses)
    int content_area_y = content_y + title_height;
    int content_area_h = content_height_ - title_height;
    
    // If there's a widget content, render it
    if (content_widget_) {
        // Set widget position and size to fit content area
        int widget_width = content_width_ - 2 * padding_;
        int widget_height = content_area_h - 2 * padding_;
        
        content_widget_->SetPosition(content_x + padding_, content_area_y + padding_);
        content_widget_->SetSize(widget_width, widget_height);
        
        // Calculate layout for containers (VBox/HBox/etc)
        content_widget_->CalculateSize(widget_width, widget_height);
        
        // Render the widget
        content_widget_->Render(cr);
    } else {
        // Fallback to custom RenderContent for subclasses
        RenderContent(cr, content_x + padding_, content_area_y + padding_,
                     content_width_ - 2 * padding_, 
                     content_area_h - 2 * padding_);
    }
}

void Modal::RenderContent(cairo_t* cr, int content_x, int content_y,
                         int content_w, int content_h) {
    // Default implementation - just show placeholder text
    cairo_select_font_face(cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size_);
    cairo_set_source_rgba(cr, text_color_[0], text_color_[1],
                         text_color_[2], text_color_[3]);
    
    cairo_move_to(cr, content_x, content_y + font_size_);
    cairo_show_text(cr, "Modal content (override RenderContent to customize)");
}

bool Modal::HandleClick(int x, int y) {
    if (!visible_) return false;
    
    // If clicking outside content area, close modal
    if (!IsPointInContent(x, y)) {
        Hide();
        return true;
    }
    
    // Forward click to widget content if present
    if (content_widget_) {
        if (content_widget_->HandleClick(x, y)) {
            return true;
        }
    }
    
    return true; // Consumed the click
}

bool Modal::HandleKeyPress(uint32_t key, uint32_t modifiers) {
    if (!visible_) return false;
    
    // Forward to widget content first if present
    if (content_widget_) {
        // Note: Widgets don't typically have key handlers, but we can add support if needed
    }
    
    // ESC to close modal
    if (key == 9) { // XKB_KEY_Escape = 9
        Hide();
        return true;
    }
    
    return false;
}

bool Modal::HandleHover(int x, int y) {
    if (!visible_) return false;
    
    // Forward hover to widget content if present
    if (content_widget_ && IsPointInContent(x, y)) {
        return content_widget_->HandleHover(x, y);
    }
    
    return false;
}

bool Modal::HandleScroll(int x, int y, double delta_x, double delta_y) {
    if (!visible_) return false;
    
    // Forward scroll to widget content if present and pointer is over modal
    if (content_widget_ && IsPointInContent(x, y)) {
        return content_widget_->HandleScroll(x, y, delta_x, delta_y);
    }
    
    return false;
}

bool Modal::IsPointInContent(int x, int y) const {
    int content_x = (screen_width_ - content_width_) / 2;
    int content_y = (screen_height_ - content_height_) / 2;
    
    return x >= content_x && x < content_x + content_width_ &&
           y >= content_y && y < content_y + content_height_;
}

void Modal::DrawRoundedRectangle(cairo_t* cr, double x, double y,
                                double width, double height, double radius) {
    double degrees = M_PI / 180.0;
    
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

} // namespace UI
} // namespace Leviathan
