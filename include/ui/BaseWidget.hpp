#pragma once

#include <atomic>
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
    Widget() : x_(0), y_(0), width_(0), height_(0), visible_(true), dirty_(true), needs_paint_(true) {}
    virtual ~Widget() = default;
    
    // Layout calculation - called on background thread or when size changes
    // This is PURE CALCULATION - no rendering
    virtual void CalculateSize(int available_width, int available_height) = 0;
    
    // Rendering - called on main Wayland thread with Cairo context
    // This only DRAWS using pre-calculated data
    virtual void Render(cairo_t* cr) = 0;
    
    // Position and size (set by parent container during layout)
    // These are only modified on main thread during layout, so no locking needed
    void SetPosition(int x, int y) { 
        x_ = x; 
        y_ = y; 
    }
    
    void SetSize(int width, int height) {
        width_ = width;
        height_ = height;
    }
    
    int GetX() const { return x_; }
    int GetY() const { return y_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    
    // Visibility (main thread only)
    void SetVisible(bool visible) { 
        visible_ = visible; 
        MarkNeedsPaint();  // Use Flutter-style (propagates to children if container)
    }
    bool IsVisible() const { 
        return visible_; 
    }
    
    // Dirty flag - indicates widget needs re-render (legacy, being replaced by needs_paint_)
    // Uses atomic operations for thread-safe flag setting from background threads
    void MarkDirty() { 
        dirty_.store(true, std::memory_order_relaxed);
        needs_paint_.store(true, std::memory_order_relaxed);
    }
    bool IsDirty() const { 
        return dirty_.load(std::memory_order_relaxed);
    }
    void ClearDirty() { 
        dirty_.store(false, std::memory_order_relaxed);
        needs_paint_.store(false, std::memory_order_relaxed);
    }
    
    // Flutter-style dirty tracking
    // When a widget needs repainting, ALL its children must also repaint
    // (children might depend on parent state)
    // Thread-safe: background threads can mark dirty, main thread clears
    virtual void MarkNeedsPaint() {
        needs_paint_.store(true, std::memory_order_relaxed);
        dirty_.store(true, std::memory_order_relaxed);  // Keep legacy flag in sync
        // Note: Container override will propagate to children
    }
    
    bool NeedsPaint() const {
        return needs_paint_.load(std::memory_order_relaxed);
    }
    
    void ClearNeedsPaint() {
        needs_paint_.store(false, std::memory_order_relaxed);
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
        // No lock needed - all input handling is on main thread
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
    int x_, y_;
    int width_, height_;
    bool visible_;
    std::atomic<bool> dirty_;           // Legacy dirty flag (being replaced by needs_paint_)
    std::atomic<bool> needs_paint_;     // Flutter-style flag: if true, this widget and all children need repainting
    Container* parent_ = nullptr;
    std::function<void()> render_callback_;  // Callback to trigger re-render
};

} // namespace UI
} // namespace Leviathan
