#pragma once

#include "ui/BaseWidget.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace Leviathan {
namespace UI {

// Forward declaration
class Container;

// Label - displays text
class Label : public Widget {
public:
    Label(const std::string& text = "") 
        : text_(text), 
          font_size_(12),
          text_color_{1.0, 1.0, 1.0, 1.0},  // White
          bg_color_{0.0, 0.0, 0.0, 0.0}      // Transparent
    {}
    
    // Update text from background thread
    void SetText(const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (text_ != text) {
            text_ = text;
            dirty_ = true;
        }
    }
    
    std::string GetText() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return text_;
    }
    
    void SetFontSize(int size) {
        std::lock_guard<std::mutex> lock(mutex_);
        font_size_ = size;
        dirty_ = true;
    }
    
    void SetTextColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        text_color_[0] = r;
        text_color_[1] = g;
        text_color_[2] = b;
        text_color_[3] = a;
        dirty_ = true;
    }
    
    void SetBackgroundColor(double r, double g, double b, double a = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
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
    double text_color_[4];
    double bg_color_[4];
    int padding_ = 4;
};

// Button - clickable widget (for future interactive status bar)
class Button : public Widget {
public:
    Button(const std::string& text = "")
        : text_(text),
          font_size_(12),
          text_color_{1.0, 1.0, 1.0, 1.0},
          bg_color_{0.2, 0.2, 0.2, 1.0},
          hover_color_{0.3, 0.3, 0.3, 1.0},
          hovered_(false)
    {}
    
    void SetText(const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (text_ != text) {
            text_ = text;
            dirty_ = true;
        }
    }
    
    void SetOnClick(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        on_click_ = callback;
    }
    
    void SetHovered(bool hovered) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (hovered_ != hovered) {
            hovered_ = hovered;
            dirty_ = true;
        }
    }
    
    void Click() {
        std::lock_guard<std::mutex> lock(mutex_);
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
    int padding_ = 8;
};

// Container base class for HBox and VBox
class Container : public Widget {
public:
    Container() : spacing_(0) {}
    virtual ~Container() = default;
    
    void AddChild(std::shared_ptr<Widget> child) {
        std::lock_guard<std::mutex> lock(mutex_);
        child->SetParent(this);
        children_.push_back(child);
        dirty_ = true;
    }
    
    void RemoveChild(std::shared_ptr<Widget> child) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            (*it)->SetParent(nullptr);
            children_.erase(it);
            dirty_ = true;
        }
    }
    
    void ClearChildren() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& child : children_) {
            child->SetParent(nullptr);
        }
        children_.clear();
        dirty_ = true;
    }
    
    const std::vector<std::shared_ptr<Widget>>& GetChildren() const {
        return children_;
    }
    
    void SetSpacing(int spacing) {
        std::lock_guard<std::mutex> lock(mutex_);
        spacing_ = spacing;
        dirty_ = true;
    }
    
    void Render(cairo_t* cr) override {
        if (!IsVisible()) return;
        
        // Render all children
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& child : children_) {
            if (child->IsVisible()) {
                child->Render(cr);
            }
        }
    }

protected:
    std::vector<std::shared_ptr<Widget>> children_;
    int spacing_;
};

// HBox - horizontal container
class HBox : public Container {
public:
    HBox() : align_(Align::Start) {}
    
    void SetAlign(Align align) {
        std::lock_guard<std::mutex> lock(mutex_);
        align_ = align;
        dirty_ = true;
    }
    
    void CalculateSize(int available_width, int available_height) override;

private:
    Align align_;
};

// VBox - vertical container
class VBox : public Container {
public:
    VBox() : align_(Align::Start) {}
    
    void SetAlign(Align align) {
        std::lock_guard<std::mutex> lock(mutex_);
        align_ = align;
        dirty_ = true;
    }
    
    void CalculateSize(int available_width, int available_height) override;

private:
    Align align_;
};

} // namespace UI
} // namespace Leviathan
