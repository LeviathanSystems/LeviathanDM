#include "ui/reusable-widgets/Container.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace UI {

// Flutter-style: When a container is marked dirty, ALL children must be marked too
// This is critical because children might depend on parent state
void Container::MarkNeedsPaint() {
    needs_paint_ = true;
    dirty_ = true;  // Keep legacy flag in sync
    
    // Propagate to ALL children - no questions asked
    for (auto& child : children_) {
        child->MarkNeedsPaint();
    }
}

void Container::Render(cairo_t* cr) {
    if (!IsVisible()) return;
    
    //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Container::Render - at ({}, {}) size={}x{}, children count={}, needs_paint={}", 
    //    x_, y_, width_, height_, children_.size(), needs_paint_);
    
    // Save cairo state
    cairo_save(cr);
    
    // Translate to container's position (establish local coordinate system)
    cairo_translate(cr, x_, y_);
    
    // Clip to container bounds (in local coordinates)
    cairo_rectangle(cr, 0, 0, width_, height_);
    cairo_clip(cr);
    
    // Render all visible children
    // NOTE: Selective rendering based on needs_paint_ is done at StatusBar level
    // Here we render all children that were passed down from the parent
    for (auto& child : children_) {
        if (child->IsVisible()) {
            //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  -> Rendering child at relative ({}, {})", child->GetX(), child->GetY());
            child->Render(cr);
        }
    }
    
    // Restore cairo state (remove translation and clipping)
    cairo_restore(cr);
}

bool Container::HandleClick(int click_x, int click_y) {
    // Check if click is within container bounds (in absolute coordinates)
    if (click_x < x_ || click_x >= x_ + width_ ||
        click_y < y_ || click_y >= y_ + height_) {
        return false;
    }
    
    // Transform click coordinates to local space (relative to container)
    // This matches how we render with cairo_translate(cr, x_, y_)
    int local_x = click_x - x_;
    int local_y = click_y - y_;
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Container::HandleClick at ({}, {}) transformed to local ({}, {})", 
                  click_x, click_y, local_x, local_y);
    
    // Check children in reverse order (top to bottom rendering = bottom to top for clicks)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        auto& child = *it;
        if (child->IsVisible() && child->HandleClick(local_x, local_y)) {
            return true;
        }
    }
    
    // Container itself handled the click (within bounds but no child handled it)
    return true;
}

bool Container::HandleHover(int hover_x, int hover_y) {
    // Check if hover is within container bounds (in absolute coordinates)
    if (hover_x < x_ || hover_x >= x_ + width_ ||
        hover_y < y_ || hover_y >= y_ + height_) {
        return false;
    }
    
    // Transform hover coordinates to local space (relative to container)
    int local_x = hover_x - x_;
    int local_y = hover_y - y_;
    
    // Check children in reverse order (top to bottom rendering = bottom to top for hover)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        auto& child = *it;
        if (child->IsVisible() && child->HandleHover(local_x, local_y)) {
            return true;
        }
    }
    
    return false;
}

bool Container::HandleScroll(int x, int y, double delta_x, double delta_y) {
    
    // Transform scroll coordinates to local space (same as click/hover)
    int local_x = x - x_;
    int local_y = y - y_;
    
    // Forward to children in reverse order (top-most first)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->HandleScroll(local_x, local_y, delta_x, delta_y)) {
            return true;
        }
    }
    
    return false;
}

} // namespace UI
} // namespace Leviathan
