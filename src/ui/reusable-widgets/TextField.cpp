#include "ui/reusable-widgets/TextField.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>

namespace Leviathan {
namespace UI {

void TextField::CalculateSize(int available_width, int available_height) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Create temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, font_family_.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, font_size_);
    
    // Get font metrics for consistent height
    cairo_font_extents_t font_extents;
    cairo_font_extents(temp_cr, &font_extents);
    
    // Calculate content width (use placeholder if text is empty)
    cairo_text_extents_t extents;
    const std::string& display_text = text_.empty() ? placeholder_ : text_;
    cairo_text_extents(temp_cr, display_text.c_str(), &extents);
    
    int content_width = static_cast<int>(extents.width);
    int content_height = static_cast<int>(font_extents.ascent + font_extents.descent);
    
    // Add padding
    int total_padding = padding_ * 2;
    
    // For outlined variant, add border width
    if (variant_ == Variant::Outlined) {
        total_padding += border_width_ * 2;
    }
    
    // Calculate final width with min/max constraints
    width_ = content_width + total_padding;
    width_ = std::max(width_, min_width_);
    if (max_width_ > 0) {
        width_ = std::min(width_, max_width_);
    }
    width_ = std::min(width_, available_width);
    
    // Calculate height
    height_ = content_height + total_padding;
    height_ = std::min(height_, available_height);
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
}

void TextField::Render(cairo_t* cr) {
    if (!IsVisible()) return;
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    cairo_save(cr);
    
    int content_x = x_;
    int content_y = y_;
    int content_w = width_;
    int content_h = height_;
    
    // Draw background
    if (bg_color_[3] > 0.0) {
        cairo_set_source_rgba(cr, bg_color_[0], bg_color_[1], bg_color_[2], bg_color_[3]);
        
        if (border_radius_ > 0) {
            // Rounded rectangle
            double radius = border_radius_;
            double x1 = x_;
            double y1 = y_;
            double x2 = x_ + width_;
            double y2 = y_ + height_;
            
            cairo_new_sub_path(cr);
            cairo_arc(cr, x2 - radius, y1 + radius, radius, -M_PI_2, 0);
            cairo_arc(cr, x2 - radius, y2 - radius, radius, 0, M_PI_2);
            cairo_arc(cr, x1 + radius, y2 - radius, radius, M_PI_2, M_PI);
            cairo_arc(cr, x1 + radius, y1 + radius, radius, M_PI, 3 * M_PI_2);
            cairo_close_path(cr);
            cairo_fill(cr);
        } else {
            cairo_rectangle(cr, x_, y_, width_, height_);
            cairo_fill(cr);
        }
    }
    
    // Draw border for outlined variant
    if (variant_ == Variant::Outlined && border_width_ > 0) {
        const double* border_col = is_focused_ ? focus_border_color_ : border_color_;
        cairo_set_source_rgba(cr, border_col[0], border_col[1], border_col[2], border_col[3]);
        cairo_set_line_width(cr, border_width_);
        
        if (border_radius_ > 0) {
            // Rounded rectangle
            double radius = border_radius_;
            double x1 = x_ + border_width_ / 2.0;
            double y1 = y_ + border_width_ / 2.0;
            double x2 = x_ + width_ - border_width_ / 2.0;
            double y2 = y_ + height_ - border_width_ / 2.0;
            
            cairo_new_sub_path(cr);
            cairo_arc(cr, x2 - radius, y1 + radius, radius, -M_PI_2, 0);
            cairo_arc(cr, x2 - radius, y2 - radius, radius, 0, M_PI_2);
            cairo_arc(cr, x1 + radius, y2 - radius, radius, M_PI_2, M_PI);
            cairo_arc(cr, x1 + radius, y1 + radius, radius, M_PI, 3 * M_PI_2);
            cairo_close_path(cr);
            cairo_stroke(cr);
        } else {
            cairo_rectangle(cr, x_ + border_width_ / 2.0, y_ + border_width_ / 2.0,
                          width_ - border_width_, height_ - border_width_);
            cairo_stroke(cr);
        }
        
        content_x += border_width_;
        content_y += border_width_;
        content_w -= border_width_ * 2;
        content_h -= border_width_ * 2;
    }
    
    // Clip to content area
    cairo_rectangle(cr, content_x + padding_, content_y + padding_,
                   content_w - padding_ * 2, content_h - padding_ * 2);
    cairo_clip(cr);
    
    // Set up text rendering
    cairo_select_font_face(cr, font_family_.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size_);
    
    cairo_font_extents_t font_extents;
    cairo_font_extents(cr, &font_extents);
    
    double text_x = content_x + padding_;
    double text_y = content_y + padding_ + font_extents.ascent;
    
    // Draw selection background if text is selected
    if (HasSelection()) {
        cairo_text_extents_t extents_before, extents_selection;
        
        int sel_start = std::min(selection_start_, selection_end_);
        int sel_end = std::max(selection_start_, selection_end_);
        
        std::string before_selection = text_.substr(0, sel_start);
        std::string selection = text_.substr(sel_start, sel_end - sel_start);
        
        cairo_text_extents(cr, before_selection.c_str(), &extents_before);
        cairo_text_extents(cr, selection.c_str(), &extents_selection);
        
        double sel_x = text_x + extents_before.x_advance;
        double sel_w = extents_selection.x_advance;
        
        cairo_set_source_rgba(cr, selection_color_[0], selection_color_[1],
                            selection_color_[2], selection_color_[3]);
        cairo_rectangle(cr, sel_x, content_y + padding_,
                       sel_w, content_h - padding_ * 2);
        cairo_fill(cr);
    }
    
    // Draw text or placeholder
    if (text_.empty() && !placeholder_.empty()) {
        // Draw placeholder
        cairo_set_source_rgba(cr, placeholder_color_[0], placeholder_color_[1],
                            placeholder_color_[2], placeholder_color_[3]);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, placeholder_.c_str());
    } else {
        // Draw text
        cairo_set_source_rgba(cr, text_color_[0], text_color_[1],
                            text_color_[2], text_color_[3]);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, text_.c_str());
    }
    
    // Draw cursor if focused and visible
    if (is_focused_ && cursor_visible_ && !text_.empty()) {
        cairo_text_extents_t extents;
        std::string before_cursor = text_.substr(0, cursor_pos_);
        cairo_text_extents(cr, before_cursor.c_str(), &extents);
        
        double cursor_x = text_x + extents.x_advance;
        
        cairo_set_source_rgba(cr, cursor_color_[0], cursor_color_[1],
                            cursor_color_[2], cursor_color_[3]);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, cursor_x, content_y + padding_);
        cairo_line_to(cr, cursor_x, content_y + content_h - padding_);
        cairo_stroke(cr);
    } else if (is_focused_ && cursor_visible_ && text_.empty()) {
        // Draw cursor at start when empty
        cairo_set_source_rgba(cr, cursor_color_[0], cursor_color_[1],
                            cursor_color_[2], cursor_color_[3]);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, text_x, content_y + padding_);
        cairo_line_to(cr, text_x, content_y + content_h - padding_);
        cairo_stroke(cr);
    }
    
    cairo_restore(cr);
}

bool TextField::HandleClick(int x, int y) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Check if click is within bounds
    if (x >= x_ && x <= x_ + width_ && y >= y_ && y <= y_ + height_) {
        if (!is_focused_) {
            Focus();
        }
        
        // Move cursor to click position
        MoveCursorToPosition(x);
        dirty_ = true;
        return true;
    } else {
        if (is_focused_) {
            Blur();
            dirty_ = true;
        }
    }
    
    return false;
}

bool TextField::HandleKeyPress(uint32_t key, uint32_t modifiers) {
    if (!is_focused_) return false;
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    bool ctrl = modifiers & 4;  // Control key
    bool shift = modifiers & 1; // Shift key
    
    // Handle special keys (simplified - in real implementation use proper keysyms)
    if (key == 65288) {  // Backspace
        if (HasSelection()) {
            DeleteSelection();
        } else if (cursor_pos_ > 0) {
            cursor_pos_--;
            text_.erase(cursor_pos_, 1);
            if (on_text_changed_) {
                on_text_changed_(text_);
            }
        }
        dirty_ = true;
        return true;
    } else if (key == 65535) {  // Delete
        if (HasSelection()) {
            DeleteSelection();
        } else if (cursor_pos_ < text_.length()) {
            text_.erase(cursor_pos_, 1);
            if (on_text_changed_) {
                on_text_changed_(text_);
            }
        }
        dirty_ = true;
        return true;
    } else if (key == 65293) {  // Enter/Return
        if (on_submit_) {
            on_submit_(text_);
        }
        return true;
    } else if (key == 65361) {  // Left arrow
        if (cursor_pos_ > 0) {
            cursor_pos_--;
            if (!shift) ClearSelection();
        }
        dirty_ = true;
        return true;
    } else if (key == 65363) {  // Right arrow
        if (cursor_pos_ < text_.length()) {
            cursor_pos_++;
            if (!shift) ClearSelection();
        }
        dirty_ = true;
        return true;
    } else if (key == 65360) {  // Home
        cursor_pos_ = 0;
        if (!shift) ClearSelection();
        dirty_ = true;
        return true;
    } else if (key == 65367) {  // End
        cursor_pos_ = text_.length();
        if (!shift) ClearSelection();
        dirty_ = true;
        return true;
    } else if (ctrl && key == 97) {  // Ctrl+A (Select All)
        SelectAll();
        dirty_ = true;
        return true;
    } else if (ctrl && key == 99) {  // Ctrl+C (Copy)
        CopySelection();
        return true;
    } else if (ctrl && key == 118) {  // Ctrl+V (Paste)
        PasteFromClipboard();
        dirty_ = true;
        return true;
    } else if (key >= 32 && key < 127) {  // Printable ASCII
        if (HasSelection()) {
            DeleteSelection();
        }
        InsertChar(static_cast<char>(key));
        dirty_ = true;
        return true;
    }
    
    return false;
}

void TextField::HandleTextInput(const std::string& text) {
    if (!is_focused_) return;
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Delete any selected text first
    if (HasSelection()) {
        DeleteSelection();
    }
    
    // Insert each character from the UTF-8 string
    for (char c : text) {
        if (c >= 32 && c < 127) {  // Printable ASCII
            InsertChar(c);
        }
    }
    
    dirty_ = true;
}

void TextField::UpdateCursorBlink(uint32_t current_time) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (!is_focused_) return;
    
    // Blink cursor every 500ms
    if (current_time - last_cursor_blink_ > 500) {
        cursor_visible_ = !cursor_visible_;
        last_cursor_blink_ = current_time;
        dirty_ = true;
    }
}

// Helper methods

void TextField::InsertChar(char c) {
    text_.insert(cursor_pos_, 1, c);
    cursor_pos_++;
    
    if (on_text_changed_) {
        on_text_changed_(text_);
    }
}

void TextField::DeleteChar() {
    if (cursor_pos_ > 0) {
        text_.erase(cursor_pos_ - 1, 1);
        cursor_pos_--;
        
        if (on_text_changed_) {
            on_text_changed_(text_);
        }
    }
}

void TextField::DeleteSelection() {
    if (!HasSelection()) return;
    
    int sel_start = std::min(selection_start_, selection_end_);
    int sel_end = std::max(selection_start_, selection_end_);
    
    text_.erase(sel_start, sel_end - sel_start);
    cursor_pos_ = sel_start;
    ClearSelection();
    
    if (on_text_changed_) {
        on_text_changed_(text_);
    }
}

void TextField::MoveCursor(int delta) {
    int new_pos = static_cast<int>(cursor_pos_) + delta;
    cursor_pos_ = std::max(0, std::min(new_pos, static_cast<int>(text_.length())));
}

void TextField::MoveCursorToPosition(int x) {
    // Create temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, font_family_.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, font_size_);
    
    int content_x = x_ + padding_;
    if (variant_ == Variant::Outlined) {
        content_x += border_width_;
    }
    
    int relative_x = x - content_x;
    
    // Find cursor position by measuring text width
    size_t best_pos = 0;
    int min_distance = std::abs(relative_x);
    
    for (size_t i = 0; i <= text_.length(); i++) {
        std::string substr = text_.substr(0, i);
        cairo_text_extents_t extents;
        cairo_text_extents(temp_cr, substr.c_str(), &extents);
        
        int distance = std::abs(relative_x - static_cast<int>(extents.x_advance));
        if (distance < min_distance) {
            min_distance = distance;
            best_pos = i;
        }
    }
    
    cursor_pos_ = best_pos;
    ClearSelection();
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
}

void TextField::SelectAll() {
    selection_start_ = 0;
    selection_end_ = text_.length();
    cursor_pos_ = text_.length();
}

void TextField::ClearSelection() {
    selection_start_ = -1;
    selection_end_ = -1;
}

bool TextField::HasSelection() const {
    return selection_start_ >= 0 && selection_end_ >= 0 && selection_start_ != selection_end_;
}

void TextField::CopySelection() {
    // TODO: Implement clipboard copy
    // This would require Wayland clipboard integration
}

void TextField::PasteFromClipboard() {
    // TODO: Implement clipboard paste
    // This would require Wayland clipboard integration
}

int TextField::GetCursorXPosition() const {
    // Create temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, font_family_.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, font_size_);
    
    std::string before_cursor = text_.substr(0, cursor_pos_);
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, before_cursor.c_str(), &extents);
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
    
    return static_cast<int>(extents.x_advance);
}

} // namespace UI
} // namespace Leviathan
