#include "ui/WidgetTree.hpp"
#include "ui/reusable-widgets/Container.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace UI {

WidgetTree::WidgetTree(std::shared_ptr<Widget> root)
    : root_(root) {
    if (!root_) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "WidgetTree created with null root widget!");
    }
}

bool WidgetTree::NeedsRender() const {
    if (!root_) return false;
    return CheckDirtyRecursive(root_);
}

void WidgetTree::Render(cairo_t* cr) {
    if (!root_ || !cr) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Cannot render: null root or cairo context");
        return;
    }
    
    // Only render if something is dirty
    if (!NeedsRender()) {
        return;
    }
    
    // Render the root widget (which will recursively render children)
    root_->Render(cr);
    
    // Clear all dirty flags after successful render
    ClearAllDirty();
}

void WidgetTree::MarkAllDirty() {
    if (!root_) return;
    MarkDirtyRecursive(root_);
}

void WidgetTree::ClearAllDirty() {
    if (!root_) return;
    ClearDirtyRecursive(root_);
}

size_t WidgetTree::CountDirtyWidgets() const {
    if (!root_) return 0;
    return CountDirtyRecursive(root_);
}

void WidgetTree::DebugPrintTree() const {
    if (!root_) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "WidgetTree: <empty>");
        return;
    }
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "WidgetTree structure:");
    DebugPrintRecursive(root_, 0);
}

// Private helper methods

bool WidgetTree::CheckDirtyRecursive(const std::shared_ptr<Widget>& widget) const {
    if (!widget) return false;
    
    // If this widget needs paint, return true immediately
    if (widget->NeedsPaint()) {
        return true;
    }
    
    // Check children if this is a container
    if (auto container = std::dynamic_pointer_cast<Container>(widget)) {
        for (const auto& child : container->GetChildren()) {
            if (CheckDirtyRecursive(child)) {
                return true;
            }
        }
    }
    
    return false;
}

void WidgetTree::ClearDirtyRecursive(const std::shared_ptr<Widget>& widget) {
    if (!widget) return;
    
    // Clear this widget's dirty flag
    widget->ClearNeedsPaint();
    
    // Recursively clear children if this is a container
    if (auto container = std::dynamic_pointer_cast<Container>(widget)) {
        for (const auto& child : container->GetChildren()) {
            ClearDirtyRecursive(child);
        }
    }
}

void WidgetTree::MarkDirtyRecursive(const std::shared_ptr<Widget>& widget) {
    if (!widget) return;
    
    // Mark this widget dirty
    widget->MarkNeedsPaint();
    // Note: MarkNeedsPaint() on containers already propagates to children
    // So we don't need to manually recurse here
}

size_t WidgetTree::CountDirtyRecursive(const std::shared_ptr<Widget>& widget) const {
    if (!widget) return 0;
    
    size_t count = widget->NeedsPaint() ? 1 : 0;
    
    // Count dirty children if this is a container
    if (auto container = std::dynamic_pointer_cast<Container>(widget)) {
        for (const auto& child : container->GetChildren()) {
            count += CountDirtyRecursive(child);
        }
    }
    
    return count;
}

void WidgetTree::DebugPrintRecursive(const std::shared_ptr<Widget>& widget, int depth) const {
    if (!widget) return;
    
    std::string indent(depth * 2, ' ');
    std::string dirty_marker = widget->NeedsPaint() ? " [DIRTY]" : "";
    std::string visible_marker = widget->IsVisible() ? "" : " [HIDDEN]";
    
    // Try to get a useful name for the widget
    std::string widget_type = "Widget";
    if (auto container = std::dynamic_pointer_cast<Container>(widget)) {
        widget_type = "Container";
        
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "{}└─ {} ({}x{} at {},{}){}{}",
                      indent, widget_type,
                      widget->GetWidth(), widget->GetHeight(),
                      widget->GetX(), widget->GetY(),
                      dirty_marker, visible_marker);
        
        // Print children
        for (const auto& child : container->GetChildren()) {
            DebugPrintRecursive(child, depth + 1);
        }
    } else {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "{}└─ {} ({}x{} at {},{}){}{}",
                      indent, widget_type,
                      widget->GetWidth(), widget->GetHeight(),
                      widget->GetX(), widget->GetY(),
                      dirty_marker, visible_marker);
    }
}

} // namespace UI
} // namespace Leviathan
