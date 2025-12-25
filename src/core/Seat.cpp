#include "core/Seat.hpp"
#include "core/Client.hpp"
#include "core/Events.hpp"
#include <algorithm>

namespace Leviathan {
namespace Core {

Seat::Seat()
    : focused_screen_(nullptr)
    , focused_client_(nullptr) {
}

Seat::~Seat() {
    // Note: screens and clients are owned elsewhere, we just reference them
    // Tags are now owned by LayerManager (per-screen)
}

void Seat::AddScreen(Screen* screen) {
    screens_.push_back(screen);
    if (!focused_screen_) {
        focused_screen_ = screen;
    }
}

void Seat::RemoveScreen(Screen* screen) {
    auto it = std::find(screens_.begin(), screens_.end(), screen);
    if (it != screens_.end()) {
        screens_.erase(it);
        if (focused_screen_ == screen) {
            focused_screen_ = screens_.empty() ? nullptr : screens_[0];
        }
    }
}

Screen* Seat::GetFocusedScreen() {
    return focused_screen_;
}

void Seat::AddClient(Client* client) {
    clients_.push_back(client);
    
    // TODO: Add to current screen's active tag
    // For now, clients are added directly through Server
}

void Seat::RemoveClient(Client* client) {
    auto it = std::find(clients_.begin(), clients_.end(), client);
    if (it != clients_.end()) {
        clients_.erase(it);
        
        if (focused_client_ == client) {
            focused_client_ = nullptr;
        }
        
        // TODO: Remove client from its tag (per-screen tags)
        // Tags are now managed per-screen by LayerManager
    }
}

void Seat::MoveClientToTag(Client* client, int tag_index) {
    // TODO: Update to work with per-screen tags from LayerManager
    // For now, this functionality is disabled
    if (!client) return;
}

void Seat::FocusClient(Client* client) {
    if (!client) return;
    
    // Unfocus previous
    if (focused_client_) {
        focused_client_->SetFocused(false);
    }
    
    focused_client_ = client;
    
    // TODO: Focus in current screen's active tag
}

void Seat::FocusNextClient() {
    // TODO: Update to work with per-screen tags from LayerManager
}

void Seat::FocusPrevClient() {
    // TODO: Update to work with per-screen tags from LayerManager
}

void Seat::CloseClient(Client* client) {
    if (client) {
        client->Close();
    }
}

void Seat::TileCurrentTag() {
    // Will be implemented with layout engine
    // TODO: Update to work with per-screen tags from LayerManager
}

void Seat::SetLayout(LayoutType layout) {
    // TODO: Update to work with per-screen tags from LayerManager
}

} // namespace Core
} // namespace Leviathan
