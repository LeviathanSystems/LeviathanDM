#pragma once

#include <cairo.h>
#include <cstdint>
#include <string>
#include <memory>

namespace Leviathan {
namespace UI {

// Forward declaration
class Widget;

/**
 * @brief Modal dialog overlay that displays centered content on screen
 * 
 * Modal provides a full-screen semi-transparent overlay with a centered
 * content box. Useful for:
 * - Help screens (keybinding reference)
 * - Settings dialogs
 * - Confirmation dialogs
 * - Information displays
 */
class Modal {
public:
    Modal();
    virtual ~Modal() = default;
    
    // Visibility
    void Show() { visible_ = true; }
    void Hide() { visible_ = false; }
    void Toggle() { visible_ = !visible_; }
    bool IsVisible() const { return visible_; }
    
    // Title and content
    void SetTitle(const std::string& title) { title_ = title; }
    const std::string& GetTitle() const { return title_; }
    
    // Size control
    void SetSize(int width, int height) { 
        content_width_ = width; 
        content_height_ = height; 
    }
    void GetSize(int& width, int& height) const {
        width = content_width_;
        height = content_height_;
    }
    
    // Position on screen (for rendering)
    void SetScreenSize(int width, int height) {
        screen_width_ = width;
        screen_height_ = height;
    }
    
    // Styling
    void SetPadding(int padding) { padding_ = padding; }
    void SetCornerRadius(int radius) { corner_radius_ = radius; }
    void SetFontSize(int size) { font_size_ = size; }
    void SetTitleFontSize(int size) { title_font_size_ = size; }
    
    void SetOverlayColor(double r, double g, double b, double a = 0.7) {
        overlay_color_[0] = r; overlay_color_[1] = g;
        overlay_color_[2] = b; overlay_color_[3] = a;
    }
    
    void SetBackgroundColor(double r, double g, double b, double a = 1.0) {
        background_color_[0] = r; background_color_[1] = g;
        background_color_[2] = b; background_color_[3] = a;
    }
    
    void SetTextColor(double r, double g, double b, double a = 1.0) {
        text_color_[0] = r; text_color_[1] = g;
        text_color_[2] = b; text_color_[3] = a;
    }
    
    void SetBorderColor(double r, double g, double b, double a = 1.0) {
        border_color_[0] = r; border_color_[1] = g;
        border_color_[2] = b; border_color_[3] = a;
    }
    
    // Rendering
    virtual void Render(cairo_t* cr);
    
    // Input handling
    virtual bool HandleClick(int x, int y);
    virtual bool HandleKeyPress(uint32_t key, uint32_t modifiers);
    virtual bool HandleHover(int x, int y);
    virtual bool HandleScroll(int x, int y, double delta_x, double delta_y);
    
    // Check if point is inside modal content area
    bool IsPointInContent(int x, int y) const;
    
    // Widget-based content (recommended approach)
    void SetContent(std::shared_ptr<Widget> widget) { content_widget_ = widget; }
    std::shared_ptr<Widget> GetContent() const { return content_widget_; }
    void ClearContent() { content_widget_ = nullptr; }
    
protected:
    // Override these for custom modal content
    virtual void RenderContent(cairo_t* cr, int content_x, int content_y, 
                              int content_w, int content_h);
    
    // Helper to draw rounded rectangle
    void DrawRoundedRectangle(cairo_t* cr, double x, double y, 
                             double width, double height, double radius);
    
protected:
    // Visibility
    bool visible_;
    
    // Title
    std::string title_;
    
    // Screen dimensions
    int screen_width_;
    int screen_height_;
    
    // Modal content dimensions
    int content_width_;
    int content_height_;
    
    // Styling
    int padding_;
    int corner_radius_;
    int font_size_;
    int title_font_size_;
    int border_width_;
    
    // Colors
    double overlay_color_[4];      // Semi-transparent overlay
    double background_color_[4];   // Modal background
    double text_color_[4];         // Text color
    double border_color_[4];       // Border color
    
    // Widget-based content
    std::shared_ptr<Widget> content_widget_;
};

} // namespace UI
} // namespace Leviathan
