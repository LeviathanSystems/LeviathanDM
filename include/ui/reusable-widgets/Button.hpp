#pragma once

#include "ui/BaseWidget.hpp"
#include <string>
#include <functional>

namespace Leviathan {
namespace UI {

// Button - clickable widget
class Button : public Widget {
public:
    Button(const std::string& text = "")
        : text_(text),
          font_size_(12),
          text_color_{1.0, 1.0, 1.0, 1.0},
          bg_color_{0.2, 0.2, 0.2, 1.0},
          hover_color_{0.3, 0.3, 0.3, 1.0},
          hovered_(false),
          padding_h_(8),
          padding_v_(4),
          border_radius_(4.0)
    {}
    
    void SetText(const std::string& text) {
        if (text_ != text) {
            text_ = text;
            dirty_ = true;
        }
    }
    
    void SetOnClick(std::function<void()> callback) {
        on_click_ = callback;
    }
    
    void SetHovered(bool hovered) {
        if (hovered_ != hovered) {
            hovered_ = hovered;
            dirty_ = true;
        }
    }
    
    void SetTextColor(double r, double g, double b, double a = 1.0) {
        text_color_[0] = r;
        text_color_[1] = g;
        text_color_[2] = b;
        text_color_[3] = a;
        dirty_ = true;
    }
    
    void SetBackgroundColor(double r, double g, double b, double a = 1.0) {
        bg_color_[0] = r;
        bg_color_[1] = g;
        bg_color_[2] = b;
        bg_color_[3] = a;
        dirty_ = true;
    }
    
    void SetHoverColor(double r, double g, double b, double a = 1.0) {
        hover_color_[0] = r;
        hover_color_[1] = g;
        hover_color_[2] = b;
        hover_color_[3] = a;
        dirty_ = true;
    }
    
    void SetFontSize(int size) {
        if (font_size_ != size) {
            font_size_ = size;
            dirty_ = true;
        }
    }
    
    void SetPadding(int horizontal, int vertical) {
        if (padding_h_ != horizontal || padding_v_ != vertical) {
            padding_h_ = horizontal;
            padding_v_ = vertical;
            dirty_ = true;
        }
    }
    
    void SetBorderRadius(double radius) {
        if (border_radius_ != radius) {
            border_radius_ = radius;
            dirty_ = true;
        }
    }
    
    void Click() {
        if (on_click_) {
            on_click_();
        }
    }
    
    void CalculateSize(int available_width, int available_height) override;
    void Render(cairo_t* cr) override;

private:
    std::string text_;
    int font_size_;
    double text_color_[4];
    double bg_color_[4];
    double hover_color_[4];
    bool hovered_;
    std::function<void()> on_click_;
    int padding_h_;
    int padding_v_;
    double border_radius_;
};

} // namespace UI
} // namespace Leviathan
