#include "core/Client.hpp"

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
}

namespace Leviathan {
namespace Core {

Client::Client(Wayland::View* view)
    : view_(view)
    , is_floating_(false)
    , is_fullscreen_(false)
    , is_focused_(false)
    , is_visible_(true) {
    
    UpdateTitle();
    UpdateAppId();
}

Client::~Client() {
    // View is owned and destroyed by Wayland layer
}

void Client::SetFloating(bool floating) {
    is_floating_ = floating;
    if (view_) {
        view_->is_floating = floating;
    }
}

void Client::SetFullscreen(bool fullscreen) {
    is_fullscreen_ = fullscreen;
    if (view_ && view_->xdg_toplevel) {
        wlr_xdg_toplevel_set_fullscreen(view_->xdg_toplevel, fullscreen);
        view_->is_fullscreen = fullscreen;
    }
}

bool Client::IsMapped() const {
    return view_ && view_->mapped;
}

void Client::SetVisible(bool visible) {
    is_visible_ = visible;
    if (view_ && view_->scene_tree) {
        wlr_scene_node_set_enabled(&view_->scene_tree->node, visible);
    }
}

int Client::GetX() const {
    return view_ ? view_->x : 0;
}

int Client::GetY() const {
    return view_ ? view_->y : 0;
}

int Client::GetWidth() const {
    return view_ ? view_->width : 0;
}

int Client::GetHeight() const {
    return view_ ? view_->height : 0;
}

void Client::SetPosition(int x, int y) {
    if (!view_) return;
    
    view_->x = x;
    view_->y = y;
    
    if (view_->scene_tree) {
        wlr_scene_node_set_position(&view_->scene_tree->node, x, y);
    }
}

void Client::SetSize(int width, int height) {
    if (!view_) return;
    
    view_->width = width;
    view_->height = height;
    
    if (view_->xdg_toplevel) {
        wlr_xdg_toplevel_set_size(view_->xdg_toplevel, width, height);
    }
}

void Client::SetGeometry(int x, int y, int width, int height) {
    SetPosition(x, y);
    SetSize(width, height);
}

void Client::Close() {
    if (view_ && view_->xdg_toplevel) {
        wlr_xdg_toplevel_send_close(view_->xdg_toplevel);
    }
}

void Client::Focus() {
    if (!view_ || !view_->mapped) return;
    
    is_focused_ = true;
    
    if (view_->scene_tree) {
        wlr_scene_node_raise_to_top(&view_->scene_tree->node);
    }
}

void Client::Raise() {
    if (!view_) return;
    
    if (view_->scene_tree) {
        wlr_scene_node_raise_to_top(&view_->scene_tree->node);
    }
}

void Client::UpdateTitle() {
    if (view_ && view_->xdg_toplevel && view_->xdg_toplevel->title) {
        title_ = view_->xdg_toplevel->title;
    } else {
        title_ = "Untitled";
    }
}

void Client::UpdateAppId() {
    if (view_ && view_->xdg_toplevel && view_->xdg_toplevel->app_id) {
        app_id_ = view_->xdg_toplevel->app_id;
    } else {
        app_id_ = "unknown";
    }
}

} // namespace Core
} // namespace Leviathan
