#pragma once

#include "ui/BaseWidget.hpp"
#include <memory>
#include <functional>
#include <cairo.h>

namespace Leviathan {
namespace UI {

/**
 * WidgetTree - Manages a tree of widgets with Flutter-style dirty tracking
 * 
 * Responsibilities:
 * - Track which widgets need repainting
 * - Perform selective rendering (only dirty widgets)
 * - Handle layout calculations
 * - Clear dirty flags after successful render
 * 
 * Usage:
 *   auto tree = std::make_shared<WidgetTree>(root_widget);
 *   if (tree->NeedsRender()) {
 *       tree->Render(cairo_context);
 *   }
 */
class WidgetTree {
public:
    /**
     * Create a widget tree with the given root widget
     */
    explicit WidgetTree(std::shared_ptr<Widget> root);
    ~WidgetTree() = default;

    /**
     * Check if any widget in the tree needs rendering
     * This recursively checks all widgets in the tree
     */
    bool NeedsRender() const;
    
    /**
     * Render all dirty widgets in the tree
     * This will:
     * 1. Only render widgets marked with needs_paint_ = true
     * 2. Automatically render children if parent is dirty
     * 3. Clear all dirty flags after successful render
     */
    void Render(cairo_t* cr);
    
    /**
     * Get the root widget of this tree
     */
    std::shared_ptr<Widget> GetRoot() const { return root_; }
    
    /**
     * Mark the entire tree as needing a full repaint
     * Useful when the entire UI needs to be redrawn (e.g., theme change)
     */
    void MarkAllDirty();
    
    /**
     * Clear all dirty flags in the tree
     * Normally called automatically after Render(), but can be called manually
     */
    void ClearAllDirty();
    
    /**
     * Count how many widgets need repainting
     * Useful for debugging and performance monitoring
     */
    size_t CountDirtyWidgets() const;
    
    /**
     * Print the tree structure with dirty status (for debugging)
     */
    void DebugPrintTree() const;

private:
    std::shared_ptr<Widget> root_;
    
    // Helper methods for recursive tree operations
    bool CheckDirtyRecursive(const std::shared_ptr<Widget>& widget) const;
    void ClearDirtyRecursive(const std::shared_ptr<Widget>& widget);
    void MarkDirtyRecursive(const std::shared_ptr<Widget>& widget);
    size_t CountDirtyRecursive(const std::shared_ptr<Widget>& widget) const;
    void DebugPrintRecursive(const std::shared_ptr<Widget>& widget, int depth) const;
};

} // namespace UI
} // namespace Leviathan
