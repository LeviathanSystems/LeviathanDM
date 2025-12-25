#include "core/Screen.hpp"
#include "Logger.hpp"

extern "C" {
#include <wlr/types/wlr_output.h>
}

#include <sstream>

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
    
    ParseOutputInfo();
}

void Screen::ParseOutputInfo() {
    if (!wlr_output_) {
        return;
    }
    
    // wlroots already parses EDID using libdisplay-info internally
    // and populates the make, model, serial, phys_width, phys_height fields
    if (wlr_output_->make) {
        make_ = wlr_output_->make;
    }
    
    if (wlr_output_->model) {
        model_ = wlr_output_->model;
    }
    
    if (wlr_output_->serial) {
        serial_ = wlr_output_->serial;
    }
    
    // Build description string
    std::ostringstream desc;
    if (!make_.empty() && !model_.empty()) {
        desc << make_ << " " << model_;
    } else if (!model_.empty()) {
        desc << model_;
    } else if (!make_.empty()) {
        desc << make_;
    } else {
        desc << name_;  // Fallback to connector name
    }
    
    // Add connector name in parentheses
    desc << " (" << name_ << ")";
    description_ = desc.str();
    
    LOG_INFO_FMT("Screen: {} - {}", name_, description_);
    if (!serial_.empty()) {
        LOG_DEBUG_FMT("  Serial: {}", serial_);
    }
    LOG_DEBUG_FMT("  Resolution: {}x{} @ {:.1f}x", width_, height_, scale_);
    
    // Log physical dimensions if available (from EDID)
    if (wlr_output_->phys_width > 0 && wlr_output_->phys_height > 0) {
        LOG_DEBUG_FMT("  Physical Size: {}mm x {}mm", wlr_output_->phys_width, wlr_output_->phys_height);
    }
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
