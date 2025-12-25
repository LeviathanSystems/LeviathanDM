#pragma once

#include "ui/BaseWidget.hpp"
#include <string>

namespace Leviathan {
namespace UI {

// Label - displays text
class Label : public Widget {
public:
    Label(const std::string& text = "") 
        : text_(text), 
          font_size_(12),
          font_family_("JetBrainsMono Nerd Font"),
          text_color_{1.0, 1.0, 1.0, 1.0},  // White
          bg_color_{0.0, 0.0, 0.0, 0.0}      // Transparent
    {}
    
    // Update text from background thread
    void SetText(const std::string& text) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (text_ != text) {
            text_ = text;
            dirty_ = true;
        }
    }
    
    std::string GetText() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return text_;
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
    
    void SetBackgroundColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        bg_color_[0] = r;
        bg_color_[1] = g;
        bg_color_[2] = b;
        bg_color_[3] = a;
        dirty_ = true;
    }
    
    void CalculateSize(int available_width, int available_height) override;
    void Render(cairo_t* cr) override;

private:
    std::string text_;
    int font_size_;
    std::string font_family_;
    double text_color_[4];
    double bg_color_[4];
    int padding_ = 4;
};

} // namespace UI
} // namespace Leviathan
