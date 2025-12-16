#include "core/Seat.hpp"
#include "core/Client.hpp"
#include <algorithm>

namespace Leviathan {
namespace Core {

Seat::Seat()
    : focused_screen_(nullptr)
    , focused_client_(nullptr)
    , active_tag_index_(0) {
}

Seat::~Seat() {
    // Note: screens, tags, and clients are owned elsewhere, we just reference them
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

void Seat::AddTag(Tag* tag) {
    tags_.push_back(tag);
}

Tag* Seat::GetTag(int index) {
    if (index >= 0 && index < static_cast<int>(tags_.size())) {
        return tags_[index];
    }
    return nullptr;
}

Tag* Seat::GetActiveTag() {
    return GetTag(active_tag_index_);
}

void Seat::SwitchToTag(int index) {
    if (index < 0 || index >= static_cast<int>(tags_.size())) {
        return;
    }
    
    // Hide current tag
    if (Tag* current = GetActiveTag()) {
        current->SetVisible(false);
    }
    
    // Show new tag
    active_tag_index_ = index;
    if (Tag* new_tag = GetActiveTag()) {
        new_tag->SetVisible(true);
    }
    
    // Retile
    TileCurrentTag();
}

void Seat::AddClient(Client* client) {
    clients_.push_back(client);
    
    // Add to active tag by default
    if (Tag* tag = GetActiveTag()) {
        tag->AddClient(client);
    }
}

void Seat::RemoveClient(Client* client) {
    auto it = std::find(clients_.begin(), clients_.end(), client);
    if (it != clients_.end()) {
        clients_.erase(it);
        
        if (focused_client_ == client) {
            focused_client_ = nullptr;
        }
        
        // Remove from all tags
        for (auto* tag : tags_) {
            tag->RemoveClient(client);
        }
    }
}

void Seat::MoveClientToTag(Client* client, int tag_index) {
    Tag* target_tag = GetTag(tag_index);
    if (!target_tag || !client) {
        return;
    }
    
    // Remove from current tag
    Tag* current_tag = GetActiveTag();
    if (current_tag) {
        current_tag->RemoveClient(client);
    }
    
    // Add to target tag
    target_tag->AddClient(client);
    
    // Hide client if not on active tag
    if (tag_index != active_tag_index_) {
        client->SetVisible(false);
    }
    
    // Clear focus
    if (focused_client_ == client) {
        focused_client_ = nullptr;
    }
}

void Seat::FocusClient(Client* client) {
    if (!client) return;
    
    // Unfocus previous
    if (focused_client_) {
        focused_client_->SetFocused(false);
    }
    
    focused_client_ = client;
    
    // Focus in active tag
    if (Tag* tag = GetActiveTag()) {
        tag->FocusClient(client);
    }
}

void Seat::FocusNextClient() {
    Tag* tag = GetActiveTag();
    if (!tag) return;
    
    const auto& clients = tag->GetClients();
    if (clients.empty()) return;
    
    auto* current = focused_client_;
    if (!current && !clients.empty()) {
        FocusClient(clients[0]);
        return;
    }
    
    auto it = std::find(clients.begin(), clients.end(), current);
    if (it != clients.end()) {
        ++it;
        if (it == clients.end()) it = clients.begin();
        FocusClient(*it);
    }
}

void Seat::FocusPrevClient() {
    Tag* tag = GetActiveTag();
    if (!tag) return;
    
    const auto& clients = tag->GetClients();
    if (clients.empty()) return;
    
    auto* current = focused_client_;
    if (!current && !clients.empty()) {
        FocusClient(clients.back());
        return;
    }
    
    auto it = std::find(clients.begin(), clients.end(), current);
    if (it != clients.end()) {
        if (it == clients.begin()) it = clients.end();
        --it;
        FocusClient(*it);
    }
}

void Seat::CloseClient(Client* client) {
    if (client) {
        client->Close();
    }
}

void Seat::TileCurrentTag() {
    // Will be implemented with layout engine
}

void Seat::SetLayout(LayoutType layout) {
    if (Tag* tag = GetActiveTag()) {
        tag->SetLayout(layout);
        TileCurrentTag();
    }
}

} // namespace Core
} // namespace Leviathan
