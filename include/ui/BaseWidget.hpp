#pragma once

#include <mutex>
#include <cairo.h>

namespace Leviathan {
namespace UI {

// Forward declaration
class Container;

// Alignment options
enum class Align {
    Start,    // Left/Top
    Center,   // Center
    End,      // Right/Bottom
    Fill      // Stretch to fill
};

// Widget base class - all widgets inherit from this
class Widget {
public:
    Widget() : x_(0), y_(0), width_(0), height_(0), visible_(true), dirty_(false) {}
    virtual ~Widget() = default;
    
    // Layout calculation - called on background thread or when size changes
    // This is PURE CALCULATION - no rendering
    virtual void CalculateSize(int available_width, int available_height) = 0;
    
    // Rendering - called on main Wayland thread with Cairo context
    // This only DRAWS using pre-calculated data
    virtual void Render(cairo_t* cr) = 0;
    
    // Position and size (set by parent container during layout)
    void SetPosition(int x, int y) { 
        std::lock_guard<std::mutex> lock(mutex_);
        x_ = x; 
        y_ = y; 
    }
    
    void SetSize(int width, int height) {
        std::lock_guard<std::mutex> lock(mutex_);
        width_ = width;
        height_ = height;
    }
    
    int GetX() const { std::lock_guard<std::mutex> lock(mutex_); return x_; }
    int GetY() const { std::lock_guard<std::mutex> lock(mutex_); return y_; }
    int GetWidth() const { std::lock_guard<std::mutex> lock(mutex_); return width_; }
    int GetHeight() const { std::lock_guard<std::mutex> lock(mutex_); return height_; }
    
    // Visibility
    void SetVisible(bool visible) { 
        std::lock_guard<std::mutex> lock(mutex_);
        visible_ = visible; 
        dirty_ = true;
    }
    bool IsVisible() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return visible_; 
    }
    
    // Dirty flag - indicates widget needs re-render
    void MarkDirty() { 
        std::lock_guard<std::mutex> lock(mutex_);
        dirty_ = true; 
    }
    bool IsDirty() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return dirty_; 
    }
    void ClearDirty() { 
        std::lock_guard<std::mutex> lock(mutex_);
        dirty_ = false; 
    }
    
    // Parent container
    void SetParent(Container* parent) { parent_ = parent; }
    Container* GetParent() const { return parent_; }

protected:
    mutable std::mutex mutex_;  // Protects all widget data
    int x_, y_;
    int width_, height_;
    bool visible_;
    bool dirty_;
    Container* parent_ = nullptr;
};

} // namespace UI
} // namespace Leviathan
