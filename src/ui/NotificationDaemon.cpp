#include "ui/NotificationDaemon.hpp"
#include "wayland/Server.hpp"
#include "Logger.hpp"

#include <gio/gio.h>
#include <chrono>
#include <algorithm>

namespace Leviathan {
namespace UI {

// DBus introspection XML for org.freedesktop.Notifications
static const char* NOTIFICATIONS_INTROSPECTION =
    "<node>"
    "  <interface name='org.freedesktop.Notifications'>"
    "    <method name='GetCapabilities'>"
    "      <arg type='as' name='capabilities' direction='out'/>"
    "    </method>"
    "    <method name='Notify'>"
    "      <arg type='s' name='app_name' direction='in'/>"
    "      <arg type='u' name='replaces_id' direction='in'/>"
    "      <arg type='s' name='app_icon' direction='in'/>"
    "      <arg type='s' name='summary' direction='in'/>"
    "      <arg type='s' name='body' direction='in'/>"
    "      <arg type='as' name='actions' direction='in'/>"
    "      <arg type='a{sv}' name='hints' direction='in'/>"
    "      <arg type='i' name='expire_timeout' direction='in'/>"
    "      <arg type='u' name='id' direction='out'/>"
    "    </method>"
    "    <method name='CloseNotification'>"
    "      <arg type='u' name='id' direction='in'/>"
    "    </method>"
    "    <method name='GetServerInformation'>"
    "      <arg type='s' name='name' direction='out'/>"
    "      <arg type='s' name='vendor' direction='out'/>"
    "      <arg type='s' name='version' direction='out'/>"
    "      <arg type='s' name='spec_version' direction='out'/>"
    "    </method>"
    "    <signal name='NotificationClosed'>"
    "      <arg type='u' name='id'/>"
    "      <arg type='u' name='reason'/>"
    "    </signal>"
    "    <signal name='ActionInvoked'>"
    "      <arg type='u' name='id'/>"
    "      <arg type='s' name='action_key'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

// NotificationData destructor - clean up GVariant hints
NotificationData::~NotificationData() {
    for (auto& [key, variant] : hints) {
        if (variant) {
            g_variant_unref(variant);
        }
    }
    hints.clear();
}

// Constructor
NotificationDaemon::NotificationDaemon(Wayland::Server* server)
    : DBusHelper(),
      server_(server),
      bus_name_id_(0),
      registration_id_(0),
      next_id_(1) {
}

// Destructor
NotificationDaemon::~NotificationDaemon() {
    Shutdown();
}

// Initialize
bool NotificationDaemon::Initialize() {
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Initializing notification daemon");
    
    // Connect to session bus using DBusHelper
    if (!ConnectToSessionBus()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to connect to session bus");
        return false;
    }
    
    // Parse introspection data
    GError* error = nullptr;
    GDBusNodeInfo* introspection_data = g_dbus_node_info_new_for_xml(
        NOTIFICATIONS_INTROSPECTION, &error);
    
    if (error) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to parse introspection XML: {}", error->message);
        g_error_free(error);
        return false;
    }
    
    // Register object on the connection from DBusHelper
    static const GDBusInterfaceVTable interface_vtable = {
        HandleMethodCall,
        nullptr,  // GetProperty
        nullptr   // SetProperty
    };
    
    registration_id_ = g_dbus_connection_register_object(
        GetConnection(),
        "/org/freedesktop/Notifications",
        introspection_data->interfaces[0],
        &interface_vtable,
        this,  // user_data
        nullptr,  // user_data_free_func
        &error
    );
    
    g_dbus_node_info_unref(introspection_data);
    
    if (error) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to register object: {}", error->message);
        g_error_free(error);
        return false;
    }
    
    // Acquire bus name
    if (!AcquireDBusName()) {
        g_dbus_connection_unregister_object(GetConnection(), registration_id_);
        registration_id_ = 0;
        return false;
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Notification daemon initialized successfully");
    return true;
}

// Shutdown
void NotificationDaemon::Shutdown() {
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Shutting down notification daemon");
    
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    
    // Close all notifications
    for (auto& [id, notification] : notifications_) {
        EmitNotificationClosed(id, UNDEFINED);
    }
    notifications_.clear();
    
    // Release DBus resources
    ReleaseDBusName();
    
    if (registration_id_ > 0 && GetConnection()) {
        g_dbus_connection_unregister_object(GetConnection(), registration_id_);
        registration_id_ = 0;
    }
    
    // Disconnect from DBus (handled by DBusHelper base class)
    Disconnect();
}

// Acquire DBus name
bool NotificationDaemon::AcquireDBusName() {
    bus_name_id_ = g_bus_own_name_on_connection(
        GetConnection(),
        "org.freedesktop.Notifications",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        nullptr,  // name_acquired_handler
        nullptr,  // name_lost_handler
        nullptr,  // user_data
        nullptr   // user_data_free_func
    );
    
    if (bus_name_id_ == 0) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to acquire DBus name 'org.freedesktop.Notifications'");
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Another notification daemon may already be running");
        return false;
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Acquired DBus name 'org.freedesktop.Notifications'");
    return true;
}

// Release DBus name
void NotificationDaemon::ReleaseDBusName() {
    if (bus_name_id_ > 0) {
        g_bus_unown_name(bus_name_id_);
        bus_name_id_ = 0;
    }
}

// Handle DBus method calls
void NotificationDaemon::HandleMethodCall(
    GDBusConnection* connection,
    const char* sender,
    const char* object_path,
    const char* interface_name,
    const char* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    void* user_data)
{
    auto* daemon = static_cast<NotificationDaemon*>(user_data);
    GVariant* result = nullptr;
    
    try {
        if (g_strcmp0(method_name, "GetCapabilities") == 0) {
            result = daemon->HandleGetCapabilities();
        }
        else if (g_strcmp0(method_name, "Notify") == 0) {
            result = daemon->HandleNotify(parameters);
        }
        else if (g_strcmp0(method_name, "CloseNotification") == 0) {
            daemon->HandleCloseNotification(parameters);
            result = nullptr;  // void return
        }
        else if (g_strcmp0(method_name, "GetServerInformation") == 0) {
            result = daemon->HandleGetServerInformation();
        }
        else {
            g_dbus_method_invocation_return_error(
                invocation,
                G_DBUS_ERROR,
                G_DBUS_ERROR_UNKNOWN_METHOD,
                "Unknown method: %s",
                method_name
            );
            return;
        }
        
        if (result) {
            g_dbus_method_invocation_return_value(invocation, result);
        } else {
            g_dbus_method_invocation_return_value(invocation, nullptr);
        }
    }
    catch (const std::exception& e) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Exception in notification method {}: {}", method_name, e.what());
        g_dbus_method_invocation_return_error(
            invocation,
            G_DBUS_ERROR,
            G_DBUS_ERROR_FAILED,
            "Internal error: %s",
            e.what()
        );
    }
}

// GetCapabilities implementation
GVariant* NotificationDaemon::HandleGetCapabilities() {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    
    // Capabilities we support
    g_variant_builder_add(&builder, "s", "body");                // Body text
    g_variant_builder_add(&builder, "s", "body-markup");         // Pango markup in body
    g_variant_builder_add(&builder, "s", "body-hyperlinks");     // Hyperlinks in body
    g_variant_builder_add(&builder, "s", "icon-static");         // Static icons
    g_variant_builder_add(&builder, "s", "actions");             // Action buttons
    g_variant_builder_add(&builder, "s", "persistence");         // Can keep notifications
    g_variant_builder_add(&builder, "s", "action-icons");        // Icons on action buttons
    
    return g_variant_builder_end(&builder);
}

// Notify implementation
GVariant* NotificationDaemon::HandleNotify(GVariant* parameters) {
    const char* app_name = nullptr;
    uint32_t replaces_id = 0;
    const char* app_icon = nullptr;
    const char* summary = nullptr;
    const char* body = nullptr;
    GVariantIter* actions_iter = nullptr;
    GVariant* hints_variant = nullptr;
    int32_t expire_timeout = 0;
    
    // Parse parameters: susssasa{sv}i
    g_variant_get(parameters, "(&su&s&s&sas@a{sv}i)",
                  &app_name, &replaces_id, &app_icon,
                  &summary, &body, &actions_iter,
                  &hints_variant, &expire_timeout);
    
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    
    // Determine notification ID
    uint32_t notification_id = replaces_id;
    NotificationData* notification = nullptr;
    
    if (replaces_id > 0) {
        // Replace existing notification
        notification = FindNotification(replaces_id);
        if (notification) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Replacing notification {}", replaces_id);
        } else {
            // ID not found, create new
            notification_id = GenerateNotificationId();
        }
    } else {
        // New notification
        notification_id = GenerateNotificationId();
    }
    
    // Create or update notification
    if (!notification) {
        notifications_[notification_id] = std::make_unique<NotificationData>();
        notification = notifications_[notification_id].get();
        notification->id = notification_id;
    }
    
    // Fill notification data
    notification->app_name = app_name ? app_name : "";
    notification->app_icon = app_icon ? app_icon : "";
    notification->summary = summary ? summary : "";
    notification->body = body ? body : "";
    notification->expire_timeout = expire_timeout;
    
    // Parse actions
    notification->actions.clear();
    if (actions_iter) {
        const char* action_str = nullptr;
        while (g_variant_iter_next(actions_iter, "&s", &action_str)) {
            if (action_str) {
                notification->actions.push_back(action_str);
            }
        }
        g_variant_iter_free(actions_iter);
    }
    
    // Parse hints
    if (hints_variant) {
        ParseHints(notification, hints_variant);
        g_variant_unref(hints_variant);
    }
    
    // Calculate expire time
    auto now = std::chrono::steady_clock::now();
    notification->created_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    int32_t timeout = expire_timeout;
    if (timeout < 0) {
        timeout = DEFAULT_TIMEOUT_MS;
    }
    
    if (timeout == 0 || notification->is_resident) {
        notification->expire_time = 0;  // Never expire
    } else {
        notification->expire_time = notification->created_time + timeout;
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Notification {}: '{}' from {}", 
                 notification_id, notification->summary, notification->app_name);
    
    // Render notification
    RenderNotification(notification);
    
    // Notify callback
    if (notification_callback_) {
        notification_callback_();
    }
    
    return g_variant_new("(u)", notification_id);
}

// CloseNotification implementation
void NotificationDaemon::HandleCloseNotification(GVariant* parameters) {
    uint32_t id = 0;
    g_variant_get(parameters, "(u)", &id);
    
    CloseNotification(id, CLOSE_CALLED);
}

// GetServerInformation implementation
GVariant* NotificationDaemon::HandleGetServerInformation() {
    return g_variant_new("(ssss)",
                        "LeviathanDM",          // name
                        "LeviathanSystems",     // vendor
                        "1.0",                  // version
                        "1.2");                 // spec_version
}

// Close notification
void NotificationDaemon::CloseNotification(uint32_t id, CloseReason reason) {
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    
    auto it = notifications_.find(id);
    if (it == notifications_.end()) {
        return;
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Closing notification {} (reason: {})", id, static_cast<int>(reason));
    
    // TODO: Animate out / destroy Wayland surface
    
    EmitNotificationClosed(id, reason);
    notifications_.erase(it);
    
    if (notification_callback_) {
        notification_callback_();
    }
    
    UpdateNotificationPositions();
}

// Emit NotificationClosed signal
void NotificationDaemon::EmitNotificationClosed(uint32_t id, CloseReason reason) {
    if (!GetConnection()) return;
    
    GError* error = nullptr;
    g_dbus_connection_emit_signal(
        GetConnection(),
        nullptr,  // destination (broadcast)
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "NotificationClosed",
        g_variant_new("(uu)", id, static_cast<uint32_t>(reason)),
        &error
    );
    
    if (error) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to emit NotificationClosed: {}", error->message);
        g_error_free(error);
    }
}

// Emit ActionInvoked signal
void NotificationDaemon::EmitActionInvoked(uint32_t id, const std::string& action_key) {
    if (!GetConnection()) return;
    
    GError* error = nullptr;
    g_dbus_connection_emit_signal(
        GetConnection(),
        nullptr,  // destination (broadcast)
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "ActionInvoked",
        g_variant_new("(us)", id, action_key.c_str()),
        &error
    );
    
    if (error) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to emit ActionInvoked: {}", error->message);
        g_error_free(error);
    }
}

// Generate unique notification ID
uint32_t NotificationDaemon::GenerateNotificationId() {
    uint32_t id = next_id_++;
    if (next_id_ == 0) {
        next_id_ = 1;  // Skip 0
    }
    return id;
}

// Find notification by ID
NotificationData* NotificationDaemon::FindNotification(uint32_t id) {
    auto it = notifications_.find(id);
    return (it != notifications_.end()) ? it->second.get() : nullptr;
}

// Parse hints from GVariant
void NotificationDaemon::ParseHints(NotificationData* notification, GVariant* hints_variant) {
    GVariantIter iter;
    const char* key = nullptr;
    GVariant* value = nullptr;
    
    g_variant_iter_init(&iter, hints_variant);
    while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
        if (g_strcmp0(key, "urgency") == 0) {
            notification->urgency = g_variant_get_byte(value);
        }
        else if (g_strcmp0(key, "resident") == 0) {
            notification->is_resident = g_variant_get_boolean(value);
        }
        
        // Store all hints for potential future use
        notification->hints[key] = g_variant_ref(value);
        g_variant_unref(value);
    }
}

// Update - process expired notifications
void NotificationDaemon::Update() {
    ProcessExpiredNotifications();
}

// Process expired notifications
void NotificationDaemon::ProcessExpiredNotifications() {
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::vector<uint32_t> expired_ids;
    
    for (const auto& [id, notification] : notifications_) {
        if (notification->expire_time > 0 && current_time >= notification->expire_time) {
            expired_ids.push_back(id);
        }
    }
    
    for (uint32_t id : expired_ids) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Notification {} expired", id);
        auto it = notifications_.find(id);
        if (it != notifications_.end()) {
            EmitNotificationClosed(id, EXPIRED);
            notifications_.erase(it);
        }
    }
    
    if (!expired_ids.empty()) {
        if (notification_callback_) {
            notification_callback_();
        }
        UpdateNotificationPositions();
    }
}

// Get active notifications
std::vector<NotificationData*> NotificationDaemon::GetActiveNotifications() const {
    std::lock_guard<std::mutex> lock(notifications_mutex_);
    
    std::vector<NotificationData*> result;
    result.reserve(notifications_.size());
    
    for (const auto& [id, notification] : notifications_) {
        result.push_back(notification.get());
    }
    
    return result;
}

// Set notification callback
void NotificationDaemon::SetNotificationCallback(std::function<void()> callback) {
    notification_callback_ = callback;
}

// Render notification (stub - will be implemented with Wayland layer surface)
void NotificationDaemon::RenderNotification(NotificationData* notification) {
    // TODO: Create Wayland layer shell surface for notification
    // TODO: Render notification content using Cairo
    // TODO: Position based on screen size and existing notifications
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Rendering notification {}: {}", 
                  notification->id, notification->summary);
}

// Update notification positions
void NotificationDaemon::UpdateNotificationPositions() {
    // TODO: Re-position all notification surfaces
    // Stack them vertically with spacing
}

// OnConnected override - called by DBusHelper after connection established
void NotificationDaemon::OnConnected() {
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "NotificationDaemon: DBus connection established");
}

} // namespace UI
} // namespace Leviathan
