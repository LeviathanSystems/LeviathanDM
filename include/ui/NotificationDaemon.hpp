#pragma once

#include "ui/DBusHelper.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <functional>
#include <cstdint>

// Forward declare GLib/DBus types
typedef struct _GDBusMethodInvocation GDBusMethodInvocation;
typedef struct _GVariant GVariant;
typedef unsigned int guint;

namespace Leviathan {

// Forward declarations
namespace Wayland { class Server; }

namespace UI {

/**
 * @brief Represents a single notification
 */
struct NotificationData {
    uint32_t id;
    std::string app_name;
    std::string app_icon;
    std::string summary;
    std::string body;
    std::vector<std::string> actions;  // Pairs: action_id, localized_string
    std::map<std::string, GVariant*> hints;
    int32_t expire_timeout;  // Milliseconds, -1 = default, 0 = never
    
    // Display state
    uint64_t created_time;
    uint64_t expire_time;
    bool is_resident;  // From hints["resident"]
    uint8_t urgency;   // 0=low, 1=normal, 2=critical
    
    ~NotificationData();
};

/**
 * @brief Notification daemon implementing org.freedesktop.Notifications spec
 * 
 * Provides a system notification service that applications can use via DBus.
 * Unlike AwesomeWM which doesn't have a built-in notification daemon,
 * LeviathanDM includes one as part of the compositor.
 * 
 * Implements the Desktop Notifications Specification v1.2:
 * https://specifications.freedesktop.org/notification-spec/latest/
 * 
 * DBus Interface:
 *   Bus Name: org.freedesktop.Notifications
 *   Object Path: /org/freedesktop/Notifications
 *   Interface: org.freedesktop.Notifications
 * 
 * Methods:
 *   GetCapabilities() -> as
 *   Notify(susssasa{sv}i) -> u
 *   CloseNotification(u) -> void
 *   GetServerInformation() -> (ssss)
 * 
 * Signals:
 *   NotificationClosed(uu)  - id, reason
 *   ActionInvoked(us)       - id, action_key
 * 
 * Example usage from shell:
 *   notify-send "Hello" "This is a notification"
 *   notify-send -u critical "Alert" "Something important"
 *   notify-send -i battery-low "Battery" "10% remaining"
 */
class NotificationDaemon : public DBusHelper {
public:
    /**
     * @brief Reasons for notification closure
     */
    enum CloseReason : uint32_t {
        EXPIRED = 1,        // Notification expired
        DISMISSED = 2,      // Dismissed by user
        CLOSE_CALLED = 3,   // CloseNotification was called
        UNDEFINED = 4       // Undefined/reserved
    };
    
    explicit NotificationDaemon(Wayland::Server* server);
    ~NotificationDaemon();
    
    /**
     * @brief Initialize the notification daemon and register on DBus
     * @return true if successful
     */
    bool Initialize();
    
    /**
     * @brief Shutdown and release DBus name
     */
    void Shutdown();
    
    /**
     * @brief Update notifications (check for expired, trigger renders)
     * Call this from the main loop periodically
     */
    void Update();
    
    /**
     * @brief Get all active notifications
     */
    std::vector<NotificationData*> GetActiveNotifications() const;
    
    /**
     * @brief Close a notification programmatically
     */
    void CloseNotification(uint32_t id, CloseReason reason);
    
    /**
     * @brief Set callback for when notifications change (for status bar widgets)
     */
    void SetNotificationCallback(std::function<void()> callback);
    
protected:
    // Override from DBusHelper - called after connection established
    void OnConnected() override;
    
private:
    // DBus name ownership
    bool AcquireDBusName();
    void ReleaseDBusName();
    
    // DBus method handlers
    static void HandleMethodCall(GDBusConnection* connection,
                                const char* sender,
                                const char* object_path,
                                const char* interface_name,
                                const char* method_name,
                                GVariant* parameters,
                                GDBusMethodInvocation* invocation,
                                void* user_data);
    
    // Method implementations
    GVariant* HandleGetCapabilities();
    GVariant* HandleNotify(GVariant* parameters);
    void HandleCloseNotification(GVariant* parameters);
    GVariant* HandleGetServerInformation();
    
    // Signal emission
    void EmitNotificationClosed(uint32_t id, CloseReason reason);
    void EmitActionInvoked(uint32_t id, const std::string& action_key);
    
    // Notification management
    uint32_t GenerateNotificationId();
    NotificationData* FindNotification(uint32_t id);
    void RemoveNotification(uint32_t id);
    void ProcessExpiredNotifications();
    
    // Rendering (will be called to create Wayland surfaces)
    void RenderNotification(NotificationData* notification);
    void UpdateNotificationPositions();
    
    // Helper to parse hints
    void ParseHints(NotificationData* notification, GVariant* hints_variant);
    
    Wayland::Server* server_;
    guint bus_name_id_;
    guint registration_id_;
    
    // Notification storage
    std::map<uint32_t, std::unique_ptr<NotificationData>> notifications_;
    uint32_t next_id_;
    
    // Callback for notification changes
    std::function<void()> notification_callback_;
    
    // Thread safety (use DBusHelper's mutex via GetMutex())
    mutable std::mutex notifications_mutex_;
    
    // Configuration (can be made configurable later)
    static constexpr int32_t DEFAULT_TIMEOUT_MS = 5000;  // 5 seconds
    static constexpr int NOTIFICATION_WIDTH = 400;
    static constexpr int NOTIFICATION_HEIGHT = 100;
    static constexpr int NOTIFICATION_SPACING = 10;
    static constexpr int NOTIFICATION_MARGIN = 10;
};

} // namespace UI
} // namespace Leviathan
