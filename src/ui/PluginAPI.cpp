#include "ui/PluginAPI.hpp"
#include "core/Tag.hpp"
#include "core/Client.hpp"
#include "core/Screen.hpp"

namespace Leviathan {
namespace UI {
namespace Plugin {

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
