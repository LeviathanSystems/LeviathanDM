#pragma once

#include "ui/BaseWidget.hpp"
#include <string>
#include <functional>

namespace Leviathan {
namespace UI {

// TextField - text input widget with standard and outlined variants
class TextField : public Widget {
public:
    enum class Variant {
        Standard,   // No border, just input area
        Outlined    // Visible border around input
    };
    
    TextField(const std::string& placeholder = "", Variant variant = Variant::Standard)
        : variant_(variant),
          text_(""),
          placeholder_(placeholder),
          font_size_(12),
          font_family_("JetBrainsMono Nerd Font"),
          text_color_{1.0, 1.0, 1.0, 1.0},              // White
          placeholder_color_{0.6, 0.6, 0.6, 1.0},       // Gray
          bg_color_{0.15, 0.15, 0.15, 1.0},             // Dark gray background
          border_color_{0.4, 0.4, 0.4, 1.0},            // Gray border
          focus_border_color_{0.3, 0.5, 0.8, 1.0},      // Blue when focused
          cursor_color_{1.0, 1.0, 1.0, 1.0},            // White cursor
          selection_color_{0.3, 0.5, 0.8, 0.3},         // Semi-transparent blue
          is_focused_(false),
          cursor_pos_(0),
          selection_start_(-1),
          selection_end_(-1),
          cursor_visible_(true),
          last_cursor_blink_(0),
          padding_(8),
          border_width_(1),
          border_radius_(4),
          min_width_(100),
          max_width_(-1)  // -1 means no max width
    {}
    
    // Text manipulation
    void SetText(const std::string& text) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (text_ != text) {
            text_ = text;
            cursor_pos_ = text_.length();
            ClearSelection();
            dirty_ = true;
            
            if (on_text_changed_) {
                on_text_changed_(text_);
            }
        }
    }
    
    std::string GetText() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return text_;
    }
    
    void SetPlaceholder(const std::string& placeholder) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        placeholder_ = placeholder;
        dirty_ = true;
    }
    
    // Styling
    void SetVariant(Variant variant) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        variant_ = variant;
        dirty_ = true;
    }
    
    void SetFontSize(int size) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        font_size_ = size;
        dirty_ = true;
    }
    
    void SetFontFamily(const std::string& family) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        font_family_ = family;
        dirty_ = true;
    }
    
    void SetTextColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        text_color_[0] = r;
        text_color_[1] = g;
        text_color_[2] = b;
        text_color_[3] = a;
        dirty_ = true;
    }
    
    void SetPlaceholderColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        placeholder_color_[0] = r;
        placeholder_color_[1] = g;
        placeholder_color_[2] = b;
        placeholder_color_[3] = a;
        dirty_ = true;
    }
    
    void SetBackgroundColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        bg_color_[0] = r;
        bg_color_[1] = g;
        bg_color_[2] = b;
        bg_color_[3] = a;
        dirty_ = true;
    }
    
    void SetBorderColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        border_color_[0] = r;
        border_color_[1] = g;
        border_color_[2] = b;
        border_color_[3] = a;
        dirty_ = true;
    }
    
    void SetFocusBorderColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        focus_border_color_[0] = r;
        focus_border_color_[1] = g;
        focus_border_color_[2] = b;
        focus_border_color_[3] = a;
        dirty_ = true;
    }
    
    void SetMinWidth(int width) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        min_width_ = width;
        dirty_ = true;
    }
    
    void SetMaxWidth(int width) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        max_width_ = width;
        dirty_ = true;
    }
    
    void SetPadding(int padding) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        padding_ = padding;
        dirty_ = true;
    }
    
    void SetBorderWidth(int width) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        border_width_ = width;
        dirty_ = true;
    }
    
    void SetBorderRadius(int radius) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        border_radius_ = radius;
        dirty_ = true;
    }
    
    // Focus management
    void Focus() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        is_focused_ = true;
        cursor_visible_ = true;
        last_cursor_blink_ = 0;
        dirty_ = true;
        
        if (on_focus_) {
            on_focus_();
        }
    }
    
    void Blur() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        is_focused_ = false;
        ClearSelection();
        dirty_ = true;
        
        if (on_blur_) {
            on_blur_();
        }
    }
    
    bool IsFocused() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return is_focused_;
    }
    
    // Callbacks
    void SetOnTextChanged(std::function<void(const std::string&)> callback) {
        on_text_changed_ = callback;
    }
    
    void SetOnSubmit(std::function<void(const std::string&)> callback) {
        on_submit_ = callback;
    }
    
    void SetOnFocus(std::function<void()> callback) {
        on_focus_ = callback;
    }
    
    void SetOnBlur(std::function<void()> callback) {
        on_blur_ = callback;
    }
    
    // Event handlers
    bool HandleClick(int x, int y) override;
    bool HandleKeyPress(uint32_t key, uint32_t modifiers);
    void HandleTextInput(const std::string& text);
    
    void CalculateSize(int available_width, int available_height) override;
    void Render(cairo_t* cr) override;
    
    // Update cursor blink (call periodically)
    void UpdateCursorBlink(uint32_t current_time);

private:
    // Helper methods
    void InsertChar(char c);
    void DeleteChar();
    void DeleteSelection();
    void MoveCursor(int delta);
    void MoveCursorToPosition(int x);
    void SelectAll();
    void ClearSelection();
    bool HasSelection() const;
    void CopySelection();
    void PasteFromClipboard();
    int GetCursorXPosition() const;
    
    // Widget properties
    Variant variant_;
    std::string text_;
    std::string placeholder_;
    int font_size_;
    std::string font_family_;
    
    // Colors
    double text_color_[4];
    double placeholder_color_[4];
    double bg_color_[4];
    double border_color_[4];
    double focus_border_color_[4];
    double cursor_color_[4];
    double selection_color_[4];
    
    // State
    bool is_focused_;
    size_t cursor_pos_;
    int selection_start_;
    int selection_end_;
    bool cursor_visible_;
    uint32_t last_cursor_blink_;
    
    // Dimensions
    int padding_;
    int border_width_;
    int border_radius_;
    int min_width_;
    int max_width_;
    
    // Callbacks
    std::function<void(const std::string&)> on_text_changed_;
    std::function<void(const std::string&)> on_submit_;
    std::function<void()> on_focus_;
    std::function<void()> on_blur_;
};

} // namespace UI
} // namespace Leviathan
