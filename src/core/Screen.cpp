#include "core/Screen.hpp"

extern "C" {
#include <wlr/types/wlr_output.h>
}

namespace Leviathan {
namespace Core {

Screen::Screen(struct wlr_output* output)
    : wlr_output_(output)
    , name_(output->name)
    , x_(0), y_(0)
    , width_(output->width)
    , height_(output->height)
    , scale_(output->scale)
    , current_tag_(nullptr) {
}

Screen::~Screen() {
}

void Screen::SetPosition(int x, int y) {
    x_ = x;
    y_ = y;
}

void Screen::SetScale(float scale) {
    scale_ = scale;
    if (wlr_output_) {
        struct wlr_output_state state;
        wlr_output_state_init(&state);
        wlr_output_state_set_scale(&state, scale);
        wlr_output_commit_state(wlr_output_, &state);
        wlr_output_state_finish(&state);
    }
}

void Screen::ShowTag(Tag* tag) {
    // Hide previous tag's views
    if (current_tag_) {
        current_tag_->SetVisible(false);
    }
    
    // Show new tag's views
    current_tag_ = tag;
    if (current_tag_) {
        current_tag_->SetVisible(true);
    }
}

} // namespace Core
} // namespace Leviathan
