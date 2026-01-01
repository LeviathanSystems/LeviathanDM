#pragma once

#include "ui/reusable-widgets/Container.hpp"

namespace Leviathan {
namespace UI {

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
