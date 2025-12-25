#include "core/Events.hpp"
#include "Logger.hpp"
#include <algorithm>

namespace Leviathan {
namespace Core {

int EventBus::Subscribe(EventType type, EventListener listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int id = next_id_++;
    subscriptions_.push_back({id, type, listener});
    
    LOG_DEBUG_FMT("EventBus: Subscription {} added for event type {}", 
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
        LOG_DEBUG_FMT("EventBus: Subscription {} removed", subscription_id);
        subscriptions_.erase(it);
    }
}

void EventBus::Publish(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Call all listeners subscribed to this event type
    for (const auto& sub : subscriptions_) {
        if (sub.type == event.type) {
            try {
                sub.listener(event);
            } catch (const std::exception& e) {
                LOG_ERROR_FMT("EventBus: Exception in event listener: {}", e.what());
            } catch (...) {
                LOG_ERROR("EventBus: Unknown exception in event listener");
            }
        }
    }
}

void EventBus::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.clear();
    LOG_DEBUG("EventBus: All subscriptions cleared");
}

} // namespace Core
} // namespace Leviathan
