#pragma once

#include "ui/BaseWidget.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include "ui/reusable-widgets/Button.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
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
    
    void Render(cairo_t* cr) override {
        if (!IsVisible()) return;
        
        // Save cairo state and clip to container bounds
        cairo_save(cr);
        cairo_rectangle(cr, x_, y_, width_, height_);
        cairo_clip(cr);
        
        // Render all children (they will be clipped to container bounds)
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& child : children_) {
            if (child->IsVisible()) {
                child->Render(cr);
            }
        }
        
        // Restore cairo state (remove clipping)
        cairo_restore(cr);
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
        std::lock_guard<std::recursive_mutex> lock(mutex_);
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
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        align_ = align;
        dirty_ = true;
    }
    
    void CalculateSize(int available_width, int available_height) override;

private:
    Align align_;
};

} // namespace UI
} // namespace Leviathan
