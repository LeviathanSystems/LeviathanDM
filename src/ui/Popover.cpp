#include "ui/reusable-widgets/Popover.hpp"
#include "ui/BaseWidget.hpp"
#include "ui/reusable-widgets/Container.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace UI {

// Helper function to recursively recalculate all nested container positions
static void RecalculateContainerTree(Widget* widget) {
    auto* container = dynamic_cast<Container*>(widget);
    if (!container) return;
    
    // Recalculate this container's size and child positions
    container->CalculateSize(10000, 10000);
    
    // Recursively recalculate all child containers
    const auto& children = container->GetChildren();
    for (const auto& child : children) {
        RecalculateContainerTree(child.get());
    }
}

void Popover::CalculateSize() {
    // If using widget-based content, calculate based on widget
    if (content_widget_) {
        content_widget_->CalculateSize(10000, 10000);  // Large available space
        int content_width = content_widget_->GetWidth();
        int content_height = content_widget_->GetHeight();
        width_ = content_width + (padding_ * 2);
        height_ = content_height + (padding_ * 2);
        
        // Position the content widget inside the popover
        content_widget_->SetPosition(x_ + padding_, y_ + padding_);
        return;
    }
    
    // Fallback to legacy PopoverItem approach
    if (items_.empty()) {
        width_ = 200;
        height_ = 50;
        return;
    }
    
    // Calculate width based on longest text
    int max_width = 200;  // Minimum width
    // TODO: Measure actual text width with cairo
    width_ = max_width;
    
    // Calculate height based on number of items
    int content_height = 0;
    for (const auto& item : items_) {
        content_height += item_height_;
        if (item.separator_after) {
            content_height += 1;  // Separator line
        }
    }
    height_ = content_height + (padding_ * 2);
}

void Popover::Render(cairo_t* cr) {
    if (!visible_) return;
    
    LOG_DEBUG_FMT("Popover::Render - content_widget_ is {}", content_widget_ ? "NOT NULL" : "NULL");
    
    cairo_save(cr);
    
    // Draw background with rounded corners
    DrawRoundedRect(cr, x_, y_, width_, height_, 8);
    cairo_set_source_rgba(cr, background_color_[0], background_color_[1],
                        background_color_[2], background_color_[3]);
    cairo_fill(cr);
    
    // Draw border
    DrawRoundedRect(cr, x_, y_, width_, height_, 8);
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    
    // Render widget-based content if available
    if (content_widget_) {
        // Position content widget at padding offset from popover origin
        // Since StatusBar renders us to our own surface at (0,0), we establish
        // a coordinate system where children render relative to popover position
        content_widget_->SetPosition(padding_, padding_);
        
        // Recalculate the entire container tree with relative positions
        LOG_DEBUG_FMT("Popover::Render - Recalculating widget tree at ({}, {})", x_, y_);
        RecalculateContainerTree(content_widget_.get());
        
        // Translate to popover's position, then render content
        // This ensures content renders in the correct place whether popover is at (0,0) or screen coords
        cairo_translate(cr, x_, y_);
        content_widget_->Render(cr);
        
        cairo_restore(cr);
        return;
    }
    
    LOG_DEBUG("Popover::Render - No content widget, using legacy PopoverItem rendering");
    // Fallback to legacy PopoverItem rendering
    int current_y = y_ + padding_;
    for (size_t i = 0; i < items_.size(); i++) {
        const auto& item = items_[i];
        
        // Draw hover background
        if (hovered_item_ == static_cast<int>(i) && item.enabled) {
            cairo_rectangle(cr, x_ + 2, current_y, width_ - 4, item_height_);
            cairo_set_source_rgba(cr, hover_color_[0], hover_color_[1],
                                hover_color_[2], hover_color_[3]);
            cairo_fill(cr);
        }
        
        // Draw icon (if present)
        int text_x = x_ + padding_;
        if (!item.icon.empty()) {
            cairo_select_font_face(cr, "monospace", 
                                 CAIRO_FONT_SLANT_NORMAL,
                                 CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, font_size_);
            cairo_set_source_rgba(cr, text_color_[0], text_color_[1],
                                text_color_[2], text_color_[3]);
            cairo_move_to(cr, text_x, current_y + item_height_ / 2 + 5);
            cairo_show_text(cr, item.icon.c_str());
            text_x += 24;  // Space for icon
        }
        
        // Draw main text
        cairo_select_font_face(cr, "sans-serif", 
                             CAIRO_FONT_SLANT_NORMAL,
                             item.enabled ? CAIRO_FONT_WEIGHT_NORMAL : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size_);
        
        double alpha = item.enabled ? text_color_[3] : 0.5;
        cairo_set_source_rgba(cr, text_color_[0], text_color_[1],
                            text_color_[2], alpha);
        cairo_move_to(cr, text_x, current_y + item_height_ / 2 + 5);
        cairo_show_text(cr, item.text.c_str());
        
        // Draw detail text (if present)
        if (!item.detail.empty()) {
            cairo_set_font_size(cr, detail_font_size_);
            cairo_set_source_rgba(cr, detail_color_[0], detail_color_[1],
                                detail_color_[2], detail_color_[3]);
            
            // Right-aligned detail text
            cairo_text_extents_t extents;
            cairo_text_extents(cr, item.detail.c_str(), &extents);
            cairo_move_to(cr, x_ + width_ - padding_ - extents.width,
                        current_y + item_height_ / 2 + 4);
            cairo_show_text(cr, item.detail.c_str());
        }
        
        current_y += item_height_;
        
        // Draw separator if needed
        if (item.separator_after) {
            cairo_move_to(cr, x_ + padding_, current_y);
            cairo_line_to(cr, x_ + width_ - padding_, current_y);
            cairo_set_source_rgba(cr, separator_color_[0], separator_color_[1],
                                separator_color_[2], separator_color_[3]);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
            current_y += 1;
        }
    }
    
    cairo_restore(cr);
}

} // namespace UI
} // namespace Leviathan
