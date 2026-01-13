#include "core/Events.hpp"
#include "ipc/IPC.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace Core {

int EventBus::Subscribe(EventType type, EventListener listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int id = next_id_++;
    subscriptions_.push_back({id, type, listener});
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "EventBus: Subscription {} added for event type {}", 
                  id, static_cast<int>(type));
    
    return id;
}

void EventBus::Unsubscribe(int subscription_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
                          [subscription_id](const Subscription& sub) {
                              return sub.id == subscription_id;
                          });
    
    if (it != subscriptions_.end()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "EventBus: Subscription {} removed", subscription_id);
        subscriptions_.erase(it);
    }
}

void EventBus::Publish(const Event& event) {
    // If IPC broadcasting is enabled, broadcast via IPC instead
    if (ipc_server_) {
        IPC::EventMessage ipc_event;
        
        // Convert Core::EventType to IPC::EventType
        switch (event.type) {
            case EventType::TagSwitched:
                ipc_event.type = IPC::EventType::TAG_SWITCHED;
                // Add event-specific data if needed
                break;
            case EventType::ClientAdded:
                ipc_event.type = IPC::EventType::CLIENT_ADDED;
                break;
            case EventType::ClientRemoved:
                ipc_event.type = IPC::EventType::CLIENT_REMOVED;
                break;
            case EventType::LayoutChanged:
                ipc_event.type = IPC::EventType::TILING_MODE_CHANGED;
                break;
            default:
                ipc_event.type = IPC::EventType::UNKNOWN;
                break;
        }
        
        // Broadcast via IPC (asynchronous, non-blocking)
        ipc_server_->BroadcastEvent(ipc_event);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "EventBus: Event broadcast via IPC (type {})", static_cast<int>(event.type));
        return;
    }
    
    // Otherwise use the old queue-based system (for backwards compatibility)
    // Queue the event (make a copy since event may be temporary)
    std::shared_ptr<Event> event_copy;
    
    // Clone the event based on its type
    switch (event.type) {
        case EventType::TagSwitched: {
            auto& e = static_cast<const TagSwitchedEvent&>(event);
            event_copy = std::make_shared<TagSwitchedEvent>(e.old_tag, e.new_tag, e.screen);
            break;
        }
        case EventType::TagVisibilityChanged: {
            auto& e = static_cast<const TagVisibilityChangedEvent&>(event);
            event_copy = std::make_shared<TagVisibilityChangedEvent>(e.tag, e.visible);
            break;
        }
        case EventType::ClientAdded: {
            auto& e = static_cast<const ClientAddedEvent&>(event);
            event_copy = std::make_shared<ClientAddedEvent>(e.client, e.tag);
            break;
        }
        default:
            // For other event types, just copy the base (extend as needed)
            event_copy = std::make_shared<Event>();
            event_copy->type = event.type;
            break;
    }
    
    // Add to queue
    {
        std::lock_guard<std::mutex> lock(mutex_);
        event_queue_.push(event_copy);
    }
    
    // Try to start processing if not already running
    bool expected = false;
    if (processing_queue_.compare_exchange_strong(expected, true)) {
        // We acquired the processing lock, drain the queue
        ProcessEventQueue();
        // Release the lock
        processing_queue_.store(false);
    }
    // else: another thread is already processing, our event is queued
}

void EventBus::ProcessEventQueue() {
    while (true) {
        // Get next event from queue
        std::shared_ptr<Event> event;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (event_queue_.empty()) {
                break;  // Queue is empty, done
            }
            event = event_queue_.front();
            event_queue_.pop();
        }
        
        // Process this event
        std::vector<EventListener> listeners_to_call;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Collect all listeners for this event type
            for (const auto& sub : subscriptions_) {
                if (sub.type == event->type) {
                    listeners_to_call.push_back(sub.listener);
                }
            }
        } // Release mutex before calling listeners
        
        // Now call all listeners without holding the mutex
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "EventBus: Calling {} listener(s) for event type {}", 
                      listeners_to_call.size(), static_cast<int>(event->type));
        
        size_t listener_index = 0;
        for (const auto& listener : listeners_to_call) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  -> Calling listener #{}", listener_index);
            try {
                listener(*event);
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  <- Listener #{} completed", listener_index);
            } catch (const std::exception& e) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "EventBus: Exception in event listener #{}: {}", listener_index, e.what());
            } catch (...) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "EventBus: Unknown exception in event listener #{}", listener_index);
            }
            listener_index++;
        }
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "EventBus: All listeners completed for event");
    }
}

void EventBus::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.clear();
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "EventBus: All subscriptions cleared");
}

} // namespace Core
} // namespace Leviathan
