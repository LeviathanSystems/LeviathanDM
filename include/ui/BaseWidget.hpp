#pragma once

#include <mutex>
#include <functional>
#include <memory>
#include <cairo.h>
#include "../Logger.hpp"

namespace Leviathan {
namespace UI {

// Forward declarations
class Container;

// Alignment options
enum class Align {
    Start,    // Left/Top
    Center,   // Center
    End,      // Right/Bottom
    Fill,     // Stretch to fill
    Apart     // Distribute children: first at start, middle centered, last at end (requires 3+ children)
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
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        x_ = x; 
        y_ = y; 
    }
    
    void SetSize(int width, int height) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        width_ = width;
        height_ = height;
    }
    
    int GetX() const { std::lock_guard<std::recursive_mutex> lock(mutex_); return x_; }
    int GetY() const { std::lock_guard<std::recursive_mutex> lock(mutex_); return y_; }
    int GetWidth() const { std::lock_guard<std::recursive_mutex> lock(mutex_); return width_; }
    int GetHeight() const { std::lock_guard<std::recursive_mutex> lock(mutex_); return height_; }
    
    // Visibility
    void SetVisible(bool visible) { 
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        visible_ = visible; 
        dirty_ = true;
    }
    bool IsVisible() const { 
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return visible_; 
    }
    
    // Dirty flag - indicates widget needs re-render
    void MarkDirty() { 
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        dirty_ = true; 
    }
    bool IsDirty() const { 
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return dirty_; 
    }
    void ClearDirty() { 
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        dirty_ = false; 
    }
    
    // Parent container
    void SetParent(Container* parent) { parent_ = parent; }
    Container* GetParent() const { return parent_; }
    
    // Get absolute screen coordinates (accumulates parent offsets)
    // Implementations are at the end of this file after Container is fully defined
    int GetAbsoluteX() const;
    int GetAbsoluteY() const;
    void GetAbsolutePosition(int& abs_x, int& abs_y) const;
    
    // Render callback - widgets call this when they update themselves
    // This allows widgets to trigger a re-render of the entire status bar
    void SetRenderCallback(std::function<void()> callback) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        render_callback_ = callback;
    }
    
    // Trigger a re-render (called by widgets when they update)
    void RequestRender() {
        MarkDirty();
        // Note: render_callback_() is NOT called here to avoid deadlocks
        // when called from within HandleClick() which holds mutex_.
        // The dirty check timer will trigger the render instead.
    }
    
    // Mouse event handlers (virtual so widgets can override)
    // Return true if event was handled
    virtual bool HandleClick(int click_x, int click_y) {
        // Default: check if click is inside widget bounds
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (click_x >= x_ && click_x <= x_ + width_ &&
            click_y >= y_ && click_y <= y_ + height_) {
            return true;
        }
        return false;
    }
    
    virtual bool HandleHover(int hover_x, int hover_y) {
        // Default: do nothing
        // Widgets can override for custom hover behavior
        return false;
    }
    
    // Scroll handling - returns true if scroll was handled
    virtual bool HandleScroll(int x, int y, double delta_x, double delta_y) {
        // Default: do nothing
        // Widgets can override for custom scroll behavior
        return false;
    }

protected:
    mutable std::recursive_mutex mutex_;  // Protects all widget data (recursive to allow nested locks)
    int x_, y_;
    int width_, height_;
    bool visible_;
    bool dirty_;
    Container* parent_ = nullptr;
    std::function<void()> render_callback_;  // Callback to trigger re-render
};

} // namespace UI
} // namespace Leviathan
