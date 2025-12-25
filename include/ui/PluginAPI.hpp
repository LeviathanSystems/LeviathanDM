#ifndef UI_PLUGIN_API_HPP
#define UI_PLUGIN_API_HPP

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * LEVIATHAN PLUGIN API - SINGLE HEADER FOR PLUGIN DEVELOPMENT
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * This is the ONLY header plugins need to include.
 * 
 * PROVIDES:
 *   - WidgetPlugin base class for creating widgets
 *   - CompositorState interface for querying compositor
 *   - Event system for subscribing to compositor events
 *   - Helper functions for accessing compositor state (C-style API)
 *   - Export macros (EXPORT_PLUGIN_CREATE, etc.)
 * 
 * USAGE:
 *   #include "ui/PluginAPI.hpp"
 *   
 *   class MyWidget : public Leviathan::UI::WidgetPlugin {
 *       void Initialize() {
 *           // Subscribe to tag switch events
 *           sub_id_ = Leviathan::UI::Plugin::SubscribeToEvent(
 *               Leviathan::UI::Plugin::EventType::TagSwitched,
 *               [this](const Leviathan::UI::Plugin::Event& event) {
 *                   OnTagChanged(event);
 *               }
 *           );
 *       }
 *       
 *       void Update() override {
 *           auto* state = Leviathan::UI::GetCompositorState();
 *           
 *           // Query tags
 *           auto tags = state->GetTags();
 *           for (auto* tag : tags) {
 *               // Use helper functions to avoid needing full Tag class
 *               std::string name = Leviathan::UI::Plugin::GetTagName(tag);
 *               int client_count = Leviathan::UI::Plugin::GetTagClientCount(tag);
 *           }
 *       }
 *   };
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "ui/WidgetPlugin.hpp"
#include "ui/CompositorState.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Leviathan {
namespace Core {
    // Forward declarations - plugins work with opaque pointers
    class Tag;
    class Client;
    class Screen;
}

namespace UI {
namespace Plugin {

/**
 * Event types that plugins can subscribe to
 */
enum class EventType {
    TagSwitched,           // Active tag changed
    TagVisibilityChanged,  // Tag became visible/hidden
    ClientAdded,           // New client/window created
    ClientRemoved,         // Client/window destroyed
    ClientTagChanged,      // Client moved to different tag
    ClientFocused,         // Client received focus
    ScreenAdded,           // New screen connected
    ScreenRemoved,         // Screen disconnected
    LayoutChanged,         // Layout type changed
};

/**
 * Base event structure
 * Plugins receive references to these and can cast to specific types
 */
struct Event {
    EventType type;
    virtual ~Event() = default;
};

/**
 * Tag switch event
 */
struct TagSwitchedEvent : public Event {
    Core::Tag* old_tag;      // Previously active tag (may be nullptr)
    Core::Tag* new_tag;      // Newly active tag
    Core::Screen* screen;    // Screen where tag switched (may be nullptr for global)
};

/**
 * Tag visibility event
 */
struct TagVisibilityChangedEvent : public Event {
    Core::Tag* tag;
    bool visible;
};

/**
 * Client added event
 */
struct ClientAddedEvent : public Event {
    Core::Client* client;
    Core::Tag* tag;
};

/**
 * Client removed event
 */
struct ClientRemovedEvent : public Event {
    Core::Client* client;
    Core::Tag* tag;
};

/**
 * Client tag changed event
 */
struct ClientTagChangedEvent : public Event {
    Core::Client* client;
    Core::Tag* old_tag;
    Core::Tag* new_tag;
};

/**
 * Client focused event
 */
struct ClientFocusedEvent : public Event {
    Core::Client* client;  // May be nullptr if focus cleared
};

/**
 * Layout changed event
 */
struct LayoutChangedEvent : public Event {
    Core::Tag* tag;
};

/**
 * Event listener callback type
 */
using EventListener = std::function<void(const Event&)>;

/**
 * Subscribe to compositor events
 * Returns subscription ID for unsubscribing
 */
int SubscribeToEvent(EventType type, EventListener listener);

/**
 * Unsubscribe from events
 */
void UnsubscribeFromEvent(int subscription_id);

/**
 * Helper functions for querying compositor objects
 * These provide a C-style API so plugins don't need the full class definitions
 */

// Tag (workspace) queries
std::string GetTagName(Core::Tag* tag);
bool IsTagVisible(Core::Tag* tag);
int GetTagClientCount(Core::Tag* tag);
std::vector<Core::Client*> GetTagClients(Core::Tag* tag);

// Tag (workspace) actions
void SwitchToTag(int tag_index);

// Client (window) queries  
std::string GetClientTitle(Core::Client* client);
std::string GetClientAppId(Core::Client* client);
bool IsClientFloating(Core::Client* client);
bool IsClientFullscreen(Core::Client* client);

// Screen (monitor) queries
std::string GetScreenName(Core::Screen* screen);
std::string GetScreenDescription(Core::Screen* screen);
std::string GetScreenMake(Core::Screen* screen);
std::string GetScreenModel(Core::Screen* screen);
std::string GetScreenSerial(Core::Screen* screen);
int GetScreenWidth(Core::Screen* screen);
int GetScreenHeight(Core::Screen* screen);
Core::Tag* GetScreenCurrentTag(Core::Screen* screen);

} // namespace Plugin
} // namespace UI
} // namespace Leviathan

#endif // UI_PLUGIN_API_HPP
