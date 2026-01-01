#pragma once

#include "ui/BaseWidget.hpp"
#include <memory>

namespace Leviathan {
namespace UI {

/**
 * @brief A scrollable container widget (like Flutter's SingleChildScrollView)
 * 
 * ScrollView wraps a single child widget and allows vertical scrolling
 * when the child's height exceeds the viewport height.
 */
class ScrollView : public Widget {
public:
    ScrollView();
    virtual ~ScrollView() = default;
    
    // Set the child widget to be scrolled
    void SetChild(std::shared_ptr<Widget> child);
    std::shared_ptr<Widget> GetChild() const { return child_; }
    
    // Scrolling
    void ScrollBy(int delta_y);  // Scroll by delta pixels
    void ScrollTo(int y);         // Scroll to absolute position
    void ScrollToTop();
    void ScrollToBottom();
    
    // Get scroll info
    int GetScrollOffset() const { return scroll_offset_; }
    int GetMaxScrollOffset() const;
    bool CanScrollUp() const;
    bool CanScrollDown() const;
    
    // Widget overrides
    void Render(cairo_t* cr) override;
    void CalculateSize(int available_width, int available_height) override;
    bool HandleClick(int x, int y) override;
    bool HandleScroll(int x, int y, double delta_x, double delta_y) override;
    
    // Scrollbar styling
    void SetScrollbarWidth(int width) { scrollbar_width_ = width; }
    void SetScrollbarColor(double r, double g, double b, double a = 0.5) {
        scrollbar_color_[0] = r; scrollbar_color_[1] = g;
        scrollbar_color_[2] = b; scrollbar_color_[3] = a;
    }
    void ShowScrollbar(bool show) { show_scrollbar_ = show; }
    
private:
    std::shared_ptr<Widget> child_;
    int scroll_offset_ = 0;  // Current scroll position (pixels from top)
    
    // Scrollbar appearance
    int scrollbar_width_ = 8;
    double scrollbar_color_[4] = {0.5, 0.5, 0.5, 0.5};
    bool show_scrollbar_ = true;
    
    // Helper to clamp scroll offset to valid range
    void ClampScrollOffset();
};

} // namespace UI
} // namespace Leviathan
