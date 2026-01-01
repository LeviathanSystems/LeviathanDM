#include "ui/BaseWidget.hpp"
#include "ui/reusable-widgets/Container.hpp"

namespace Leviathan {
namespace UI {

int Widget::GetAbsoluteX() const {
    int abs_x = x_;
    Container* p = parent_;
    while (p) {
        abs_x += p->GetX();
        p = p->GetParent();
    }
    return abs_x;
}

int Widget::GetAbsoluteY() const {
    int abs_y = y_;
    Container* p = parent_;
    while (p) {
        abs_y += p->GetY();
        p = p->GetParent();
    }
    return abs_y;
}

void Widget::GetAbsolutePosition(int& abs_x, int& abs_y) const {
    abs_x = GetAbsoluteX();
    abs_y = GetAbsoluteY();
}

} // namespace UI
} // namespace Leviathan
