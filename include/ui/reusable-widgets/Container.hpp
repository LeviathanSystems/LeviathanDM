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
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        child->SetParent(this);
        children_.push_back(child);
        dirty_ = true;
    }
    
    void RemoveChild(std::shared_ptr<Widget> child) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            (*it)->SetParent(nullptr);
            children_.erase(it);
            dirty_ = true;
        }
    }
    
    void ClearChildren() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
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
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        spacing_ = spacing;
        dirty_ = true;
    }
    
    void Render(cairo_t* cr) override;
    
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
