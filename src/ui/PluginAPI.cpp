#include "ui/PluginAPI.hpp"
#include "core/Tag.hpp"
#include "core/Client.hpp"
#include "core/Screen.hpp"
#include "core/Events.hpp"

namespace Leviathan {
namespace UI {
namespace Plugin {

// Event subscription
int SubscribeToEvent(EventType type, EventListener listener) {
    // Convert plugin EventType to Core::EventType
    Core::EventType core_type;
    switch (type) {
        case EventType::TagSwitched:
            core_type = Core::EventType::TagSwitched;
            break;
        case EventType::TagVisibilityChanged:
            core_type = Core::EventType::TagVisibilityChanged;
            break;
        case EventType::ClientAdded:
            core_type = Core::EventType::ClientAdded;
            break;
        case EventType::ClientRemoved:
            core_type = Core::EventType::ClientRemoved;
            break;
        case EventType::ClientTagChanged:
            core_type = Core::EventType::ClientTagChanged;
            break;
        case EventType::ClientFocused:
            core_type = Core::EventType::ClientFocused;
            break;
        case EventType::ScreenAdded:
            core_type = Core::EventType::ScreenAdded;
            break;
        case EventType::ScreenRemoved:
            core_type = Core::EventType::ScreenRemoved;
            break;
        case EventType::LayoutChanged:
            core_type = Core::EventType::LayoutChanged;
            break;
        default:
            return -1;
    }
    
    // Subscribe through the event bus
    return Core::EventBus::Instance().Subscribe(core_type, [listener](const Core::Event& core_event) {
        // Convert Core::Event to Plugin::Event
        // The event structures are identical, so we can safely cast
        const auto& plugin_event = reinterpret_cast<const Event&>(core_event);
        listener(plugin_event);
    });
}

void UnsubscribeFromEvent(int subscription_id) {
    Core::EventBus::Instance().Unsubscribe(subscription_id);
}

// Tag queries
std::string GetTagName(Core::Tag* tag) {
    if (!tag) return "";
    return tag->GetName();
}

bool IsTagVisible(Core::Tag* tag) {
    if (!tag) return false;
    return tag->IsVisible();
}

int GetTagClientCount(Core::Tag* tag) {
    if (!tag) return 0;
    return static_cast<int>(tag->GetClients().size());
}

std::vector<Core::Client*> GetTagClients(Core::Tag* tag) {
    if (!tag) return {};
    return tag->GetClients();
}

// Client queries
std::string GetClientTitle(Core::Client* client) {
    if (!client) return "";
    return client->GetTitle();
}

std::string GetClientAppId(Core::Client* client) {
    if (!client) return "";
    return client->GetAppId();
}

bool IsClientFloating(Core::Client* client) {
    if (!client) return false;
    return client->IsFloating();
}

bool IsClientFullscreen(Core::Client* client) {
    if (!client) return false;
    return client->IsFullscreen();
}

// Tag actions
void SwitchToTag(int tag_index) {
    auto* compositor = UI::GetCompositorState();
    if (!compositor) {
        LOG_WARN("PluginAPI::SwitchToTag: GetCompositorState() returned nullptr");
        return;
    }
    
    compositor->SwitchToTag(tag_index);
}

// Screen queries
std::string GetScreenName(Core::Screen* screen) {
    if (!screen) return "";
    return screen->GetName();
}

int GetScreenWidth(Core::Screen* screen) {
    if (!screen) return 0;
    return screen->GetWidth();
}

int GetScreenHeight(Core::Screen* screen) {
    if (!screen) return 0;
    return screen->GetHeight();
}

Core::Tag* GetScreenCurrentTag(Core::Screen* screen) {
    if (!screen) return nullptr;
    return screen->GetCurrentTag();
}

} // namespace Plugin
} // namespace UI
} // namespace Leviathan
