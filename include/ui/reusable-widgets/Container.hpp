#pragma once

#include "ui/BaseWidget.hpp"
#include <vector>
#include <memory>
#include <algorithm>

namespace Leviathan {
namespace UI {

// Forward declaration
class Container;

// Container base class for HBox and VBox
class Container : public Widget {
public:
    Container() : spacing_(0) {}
    virtual ~Container() = default;
    
    void AddChild(std::shared_ptr<Widget> child) {
        child->SetParent(this);
        children_.push_back(child);
        MarkNeedsPaint();  // Use Flutter-style marking (propagates to all children)
    }
    
    void RemoveChild(std::shared_ptr<Widget> child) {
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            (*it)->SetParent(nullptr);
            children_.erase(it);
            MarkNeedsPaint();  // Use Flutter-style marking
        }
    }
    
    void ClearChildren() {
        for (auto& child : children_) {
            child->SetParent(nullptr);
        }
        children_.clear();
        MarkNeedsPaint();  // Use Flutter-style marking
    }
    
    const std::vector<std::shared_ptr<Widget>>& GetChildren() const {
        return children_;
    }
    
    void SetSpacing(int spacing) {
        spacing_ = spacing;
        MarkNeedsPaint();  // Use Flutter-style marking
    }
    
    void Render(cairo_t* cr) override;
    
    // Flutter-style: When container needs paint, ALL children must repaint
    void MarkNeedsPaint() override;
    
    // Override HandleClick to transform coordinates to local space
    // This matches the rendering coordinate system (cairo_translate)
    bool HandleClick(int click_x, int click_y) override;
    bool HandleHover(int hover_x, int hover_y) override;
    bool HandleScroll(int x, int y, double delta_x, double delta_y) override;

protected:
    std::vector<std::shared_ptr<Widget>> children_;
    int spacing_;
};

} // namespace UI
} // namespace Leviathan
