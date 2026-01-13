#include "core/Tag.hpp"
#include "core/Client.hpp"
#include "core/Events.hpp"
#include "Logger.hpp"

extern "C" {
#include <wlr/types/wlr_scene.h>
}

#include <algorithm>

namespace Leviathan {
namespace Core {

Tag::Tag(const std::string& name)
    : name_(name)
    , visible_(false)
    , focused_client_(nullptr)
    , layout_(LayoutType::MASTER_STACK)
    , master_count_(1)
    , master_ratio_(0.55f) {
}

Tag::~Tag() {
}

void Tag::SetVisible(bool visible) {
    if (visible_ == visible) {
        return;  // No change
    }
    
    visible_ = visible;
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Tag '{}' visibility changed to: {}", name_, visible);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Tag has {} clients", clients_.size());
    
    // Show/hide all clients in this tag
    for (auto* client : clients_) {
        auto* view = client->GetView();
        if (view && view->scene_tree) {
            wlr_scene_node_set_enabled(&view->scene_tree->node, visible);
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  - Set scene node enabled={} for view={}", 
                     visible, static_cast<void*>(view));
        }
    }
    
    // Fire visibility changed event
    TagVisibilityChangedEvent event(this, visible);
    EventBus::Instance().Publish(event);
}

void Tag::AddClient(Client* client) {
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Tag::AddClient - Adding client {:p} to tag '{}'", static_cast<void*>(client), name_);
    
    clients_.push_back(client);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Added client to tag '{}' (total clients: {})", name_, clients_.size());
    
    // If tag is visible, make sure the new client is visible too
    if (visible_) {
        auto* view = client->GetView();
        if (view && view->scene_tree) {
            wlr_scene_node_set_enabled(&view->scene_tree->node, true);
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  - Enabled scene node for new client (tag is visible)");
        }
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Tag::AddClient - About to publish ClientAddedEvent");
    
    // Fire client added event
    try {
        ClientAddedEvent event(client, this);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  - Event created: client={:p}, tag={:p}", static_cast<void*>(client), static_cast<void*>(this));
        EventBus::Instance().Publish(event);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  - ClientAddedEvent published successfully");
    } catch (const std::exception& e) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Tag::AddClient: Exception publishing event: {}", e.what());
    } catch (...) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Tag::AddClient: Unknown exception publishing event");
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Tag::AddClient - Completed");
}

void Tag::RemoveClient(Client* client) {
    auto it = std::find(clients_.begin(), clients_.end(), client);
    if (it != clients_.end()) {
        clients_.erase(it);
        if (focused_client_ == client) {
            focused_client_ = clients_.empty() ? nullptr : clients_[0];
        }
        
        // Fire client removed event
        ClientRemovedEvent event(client, this);
        EventBus::Instance().Publish(event);
    }
}

void Tag::FocusClient(Client* client) {
    if (!client || !client->IsMapped()) {
        return;
    }
    
    // Unfocus previous
    if (focused_client_) {
        focused_client_->SetFocused(false);
    }
    
    focused_client_ = client;
    focused_client_->SetFocused(true);
    focused_client_->Focus();
}

void Tag::SetLayout(LayoutType layout) {
    layout_ = layout;
}

void Tag::SetMasterCount(int count) {
    if (count >= 0) {
        master_count_ = count;
    }
}

void Tag::SetMasterRatio(float ratio) {
    if (ratio > 0.05f && ratio < 0.95f) {
        master_ratio_ = ratio;
    }
}

} // namespace Core
} // namespace Leviathan
