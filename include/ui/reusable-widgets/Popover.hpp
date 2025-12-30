#pragma once

#include <cairo.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Leviathan {
namespace UI {

// Forward declaration
class Widget;

/**
 * @brief Single item in a popover menu/list
 */
struct PopoverItem {
    std::string text;
    std::string icon;  // Optional icon text (for icon fonts or emoji)
    std::string detail;  // Optional detail/subtitle text
    std::function<void()> callback;  // Called when item is clicked
    bool enabled = true;
    bool separator_after = false;
};

/**
 * @brief Base class for popover rendering
 * 
 * Popovers are floating windows that appear near a widget when triggered.
 * They can contain:
 * - Lists of items with icons and details
 * - Custom rendered content
 * - Interactive elements (buttons, sliders, etc.)
 * 
 * Widgets can use this to show:
 * - Context menus
 * - Additional information
 * - Device lists (battery widget shows all battery devices)
 * - Quick settings
 */
class Popover {
public:
    Popover() 
        : x_(0), y_(0), 
          width_(200), height_(100),
          padding_(8), item_height_(24),
          visible_(false),
          background_color_{0.15, 0.15, 0.15, 0.95},
          text_color_{1.0, 1.0, 1.0, 1.0},
          detail_color_{0.7, 0.7, 0.7, 1.0},
          separator_color_{0.4, 0.4, 0.4, 1.0},
          hover_color_{0.25, 0.25, 0.25, 1.0},
          font_size_(12),
          detail_font_size_(10),
          hovered_item_(-1),
          content_widget_(nullptr) {}
    
    virtual ~Popover() = default;
    
    // Position and size
    void SetPosition(int x, int y) { x_ = x; y_ = y; }
    void SetSize(int width, int height) { width_ = width; height_ = height; }
    void GetPosition(int& x, int& y) const { x = x_; y = y_; }
    void GetSize(int& width, int& height) const { width = width_; height = height_; }
    
    // Visibility
    void Show() { visible_ = true; }
    void Hide() { visible_ = false; }
    void Toggle() { visible_ = !visible_; }
    bool IsVisible() const { return visible_; }
    
    // Styling
    void SetPadding(int padding) { padding_ = padding; }
    void SetItemHeight(int height) { item_height_ = height; }
    void SetFontSize(int size) { font_size_ = size; }
    void SetDetailFontSize(int size) { detail_font_size_ = size; }
    
    void SetBackgroundColor(double r, double g, double b, double a = 1.0) {
        background_color_[0] = r; background_color_[1] = g;
        background_color_[2] = b; background_color_[3] = a;
    }
    
    void SetTextColor(double r, double g, double b, double a = 1.0) {
        text_color_[0] = r; text_color_[1] = g;
        text_color_[2] = b; text_color_[3] = a;
    }
    
    // Content management
    void AddItem(const PopoverItem& item) { items_.push_back(item); }
    void ClearItems() { items_.clear(); hovered_item_ = -1; }
    const std::vector<PopoverItem>& GetItems() const { return items_; }
    
    // Widget-based content (new approach)
    void SetContent(std::shared_ptr<Widget> content) { content_widget_ = content; }
    std::shared_ptr<Widget> GetContent() const { return content_widget_; }
    void ClearContent() { content_widget_ = nullptr; }
    
    /**
     * @brief Calculate popover size based on content
     * 
     * Automatically sizes the popover to fit all items with padding.
     * Can be overridden for custom content sizing.
     */
    virtual void CalculateSize();
    
    /**
     * @brief Render the popover
     * 
     * Default implementation draws a rounded rectangle with a list of items.
     * Override for custom rendering.
     */
    virtual void Render(cairo_t* cr);
    
    /**
     * @brief Handle mouse click event
     * @return true if click was handled (inside popover bounds)
     */
    virtual bool HandleClick(int click_x, int click_y) {
        if (!visible_) return false;
        
        // Check if click is inside popover
        if (click_x < x_ || click_x > x_ + width_ ||
            click_y < y_ || click_y > y_ + height_) {
            return false;
        }
        
        // Find which item was clicked
        int relative_y = click_y - y_ - padding_;
        int current_y = 0;
        
        for (size_t i = 0; i < items_.size(); i++) {
            if (relative_y >= current_y && relative_y < current_y + item_height_) {
                if (items_[i].enabled && items_[i].callback) {
                    items_[i].callback();
                    Hide();  // Auto-hide after click
                }
                return true;
            }
            current_y += item_height_;
            if (items_[i].separator_after) {
                current_y += 1;
            }
        }
        
        return true;  // Click was inside popover, just not on an item
    }
    
    /**
     * @brief Handle mouse hover event
     * @return true if hover is inside popover bounds
     */
    virtual bool HandleHover(int hover_x, int hover_y) {
        if (!visible_) return false;
        
        // Check if hover is inside popover
        if (hover_x < x_ || hover_x > x_ + width_ ||
            hover_y < y_ || hover_y > y_ + height_) {
            hovered_item_ = -1;
            return false;
        }
        
        // Find which item is hovered
        int relative_y = hover_y - y_ - padding_;
        int current_y = 0;
        
        for (size_t i = 0; i < items_.size(); i++) {
            if (relative_y >= current_y && relative_y < current_y + item_height_) {
                hovered_item_ = static_cast<int>(i);
                return true;
            }
            current_y += item_height_;
            if (items_[i].separator_after) {
                current_y += 1;
            }
        }
        
        hovered_item_ = -1;
        return true;
    }

protected:
    /**
     * @brief Helper to draw rounded rectangle
     */
    void DrawRoundedRect(cairo_t* cr, double x, double y, double width, double height, double radius) {
        double degrees = M_PI / 180.0;
        
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
        cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
        cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
        cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
        cairo_close_path(cr);
    }

    // Position and size
    int x_, y_;
    int width_, height_;
    
    // Styling
    int padding_;
    int item_height_;
    int font_size_;
    int detail_font_size_;
    
    // State
    bool visible_;
    int hovered_item_;
    
    // Colors
    double background_color_[4];
    double text_color_[4];
    double detail_color_[4];
    double separator_color_[4];
    double hover_color_[4];
    
    // Content
    std::vector<PopoverItem> items_;
    
    // Widget-based content (new approach)
    std::shared_ptr<Widget> content_widget_;
};

} // namespace UI
} // namespace Leviathan
