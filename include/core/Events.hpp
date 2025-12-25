#ifndef CORE_EVENTS_HPP
#define CORE_EVENTS_HPP

#include <functional>
#include <vector>
#include <memory>
#include <mutex>

namespace Leviathan {
namespace Core {

// Forward declarations
class Tag;
class Client;
class Screen;

/**
 * Event types that can be subscribed to
 */
enum class EventType {
    TagSwitched,        // Active tag changed
    TagVisibilityChanged,  // Tag became visible/hidden
    ClientAdded,        // New client/window created
    ClientRemoved,      // Client/window destroyed
    ClientTagChanged,   // Client moved to different tag
    ClientFocused,      // Client received focus
    ScreenAdded,        // New screen connected
    ScreenRemoved,      // Screen disconnected
    LayoutChanged,      // Layout type changed
};

/**
 * Base event data - all events inherit from this
 */
struct Event {
    EventType type;
    virtual ~Event() = default;
};

/**
 * Tag switch event - fired when active tag changes
 */
struct TagSwitchedEvent : public Event {
    TagSwitchedEvent(Tag* old_tag, Tag* new_tag, Screen* screen)
        : old_tag(old_tag), new_tag(new_tag), screen(screen) {
        type = EventType::TagSwitched;
    }
    
    Tag* old_tag;      // Previously active tag (may be nullptr)
    Tag* new_tag;      // Newly active tag
    Screen* screen;    // Screen where tag switched (may be nullptr for global)
};

/**
 * Tag visibility event - fired when tag becomes visible/hidden
 */
struct TagVisibilityChangedEvent : public Event {
    TagVisibilityChangedEvent(Tag* tag, bool visible)
        : tag(tag), visible(visible) {
        type = EventType::TagVisibilityChanged;
    }
    
    Tag* tag;
    bool visible;
};

/**
 * Client added event
 */
struct ClientAddedEvent : public Event {
    ClientAddedEvent(Client* client, Tag* tag)
        : client(client), tag(tag) {
        type = EventType::ClientAdded;
    }
    
    Client* client;
    Tag* tag;
};

/**
 * Client removed event
 */
struct ClientRemovedEvent : public Event {
    ClientRemovedEvent(Client* client, Tag* tag)
        : client(client), tag(tag) {
        type = EventType::ClientRemoved;
    }
    
    Client* client;
    Tag* tag;
};

/**
 * Client tag changed event
 */
struct ClientTagChangedEvent : public Event {
    ClientTagChangedEvent(Client* client, Tag* old_tag, Tag* new_tag)
        : client(client), old_tag(old_tag), new_tag(new_tag) {
        type = EventType::ClientTagChanged;
    }
    
    Client* client;
    Tag* old_tag;
    Tag* new_tag;
};

/**
 * Client focused event
 */
struct ClientFocusedEvent : public Event {
    ClientFocusedEvent(Client* client)
        : client(client) {
        type = EventType::ClientFocused;
    }
    
    Client* client;  // May be nullptr if focus cleared
};

/**
 * Layout changed event
 */
struct LayoutChangedEvent : public Event {
    LayoutChangedEvent(Tag* tag)
        : tag(tag) {
        type = EventType::LayoutChanged;
    }
    
    Tag* tag;
};

/**
 * Event listener callback type
 */
using EventListener = std::function<void(const Event&)>;

/**
 * EventBus - central event dispatcher
 * Allows components to subscribe to events and receive notifications
 */
class EventBus {
public:
    static EventBus& Instance() {
        static EventBus instance;
        return instance;
    }
    
    /**
     * Subscribe to an event type
     * Returns subscription ID that can be used to unsubscribe
     */
    int Subscribe(EventType type, EventListener listener);
    
    /**
     * Unsubscribe from events using subscription ID
     */
    void Unsubscribe(int subscription_id);
    
    /**
     * Publish an event to all subscribers
     * Can be called from any thread
     */
    void Publish(const Event& event);
    
    /**
     * Clear all subscriptions (useful for cleanup)
     */
    void Clear();
    
private:
    EventBus() : next_id_(1) {}
    ~EventBus() = default;
    
    // Delete copy/move constructors
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    struct Subscription {
        int id;
        EventType type;
        EventListener listener;
    };
    
    std::vector<Subscription> subscriptions_;
    std::mutex mutex_;
    int next_id_;
};

} // namespace Core
} // namespace Leviathan

#endif // CORE_EVENTS_HPP
