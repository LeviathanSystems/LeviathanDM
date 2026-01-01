#include "ui/reusable-widgets/ScrollView.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>  // For M_PI

namespace Leviathan {
namespace UI {

ScrollView::ScrollView() 
    : Widget() {
}

void ScrollView::SetChild(std::shared_ptr<Widget> child) {
    child_ = child;
    if (child_) {
        child_->SetParent(nullptr);  // ScrollView is not a Container
    }
    // RecalculateSize will be called when SetSize is called
}

void ScrollView::ScrollBy(int delta_y) {
    scroll_offset_ += delta_y;
    ClampScrollOffset();
}

void ScrollView::ScrollTo(int y) {
    scroll_offset_ = y;
    ClampScrollOffset();
}

void ScrollView::ScrollToTop() {
    scroll_offset_ = 0;
}

void ScrollView::ScrollToBottom() {
    scroll_offset_ = GetMaxScrollOffset();
}

int ScrollView::GetMaxScrollOffset() const {
    if (!child_) return 0;
    
    int child_height = child_->GetHeight();
    int viewport_height = height_;
    
    return std::max(0, child_height - viewport_height);
}

bool ScrollView::CanScrollUp() const {
    return scroll_offset_ > 0;
}

bool ScrollView::CanScrollDown() const {
    return scroll_offset_ < GetMaxScrollOffset();
}

void ScrollView::ClampScrollOffset() {
    int max_offset = GetMaxScrollOffset();
    scroll_offset_ = std::clamp(scroll_offset_, 0, max_offset);
}

void ScrollView::Render(cairo_t* cr) {
    if (!IsVisible() || !child_) return;
    
    // Save cairo state
    cairo_save(cr);
    
    // Create clipping region for the viewport
    cairo_rectangle(cr, x_, y_, width_, height_);
    cairo_clip(cr);
    
    // Translate to apply scroll offset
    cairo_translate(cr, 0, -scroll_offset_);
    
    // Render the child
    child_->Render(cr);
    
    // Restore cairo state
    cairo_restore(cr);
    
    // Draw scrollbar if enabled and content is scrollable
    if (show_scrollbar_ && GetMaxScrollOffset() > 0) {
        int max_offset = GetMaxScrollOffset();
        int child_height = child_->GetHeight();
        
        // Calculate scrollbar dimensions
        float viewport_ratio = static_cast<float>(height_) / child_height;
        int scrollbar_height = std::max(20, static_cast<int>(height_ * viewport_ratio));
        
        // Calculate scrollbar position
        float scroll_ratio = static_cast<float>(scroll_offset_) / max_offset;
        int scrollbar_y = y_ + static_cast<int>((height_ - scrollbar_height) * scroll_ratio);
        
        int scrollbar_x = x_ + width_ - scrollbar_width_;
        
        // Draw scrollbar track (lighter)
        cairo_set_source_rgba(cr, 
            scrollbar_color_[0] * 0.3, 
            scrollbar_color_[1] * 0.3, 
            scrollbar_color_[2] * 0.3, 
            scrollbar_color_[3] * 0.5);
        cairo_rectangle(cr, scrollbar_x, y_, scrollbar_width_, height_);
        cairo_fill(cr);
        
        // Draw scrollbar thumb
        cairo_set_source_rgba(cr, 
            scrollbar_color_[0], 
            scrollbar_color_[1], 
            scrollbar_color_[2], 
            scrollbar_color_[3]);
        
        // Rounded rectangle for scrollbar
        double radius = scrollbar_width_ / 2.0;
        cairo_new_sub_path(cr);
        cairo_arc(cr, scrollbar_x + radius, scrollbar_y + radius, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, scrollbar_x + scrollbar_width_ - radius, scrollbar_y + radius, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, scrollbar_x + scrollbar_width_ - radius, scrollbar_y + scrollbar_height - radius, radius, 0, M_PI / 2);
        cairo_arc(cr, scrollbar_x + radius, scrollbar_y + scrollbar_height - radius, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);
        cairo_fill(cr);
    }
}

void ScrollView::CalculateSize(int available_width, int available_height) {
    // ScrollView takes the available space
    width_ = available_width;
    height_ = available_height;
    
    if (child_) {
        // Let child calculate its natural size (it can be taller than viewport)
        child_->CalculateSize(available_width, available_height * 2);  // Give it more space
        
        // Position child at origin (we'll translate with scroll offset)
        child_->SetPosition(x_, y_);
    }
}

bool ScrollView::HandleClick(int x, int y) {
    if (!IsVisible() || !child_) return false;
    
    // Check if click is within our bounds
    if (x < x_ || x >= x_ + width_ || y < y_ || y >= y_ + height_) {
        return false;
    }
    
    // Adjust coordinates for scroll offset
    int child_y = y + scroll_offset_;
    
    return child_->HandleClick(x, child_y);
}

bool ScrollView::HandleScroll(int x, int y, double delta_x, double delta_y) {
    if (!IsVisible() || !child_) return false;
    
    // Check if scroll is within our bounds
    if (x < x_ || x >= x_ + width_ || y < y_ || y >= y_ + height_) {
        return false;
    }
    
    // Scroll vertically (delta_y is typically -1 for up, +1 for down)
    // Multiply by scroll speed (e.g., 30 pixels per scroll step)
    int scroll_amount = static_cast<int>(delta_y * 30);
    ScrollBy(scroll_amount);
    
    return true;  // We handled the scroll
}

} // namespace UI
} // namespace Leviathan
