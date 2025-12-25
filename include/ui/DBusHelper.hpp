#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <mutex>

// Forward declare GLib/DBus types to avoid including headers here
typedef struct _GDBusConnection GDBusConnection;
typedef struct _GDBusProxy GDBusProxy;
typedef struct _GVariant GVariant;
typedef struct _GError GError;
typedef unsigned int guint;

namespace Leviathan {
namespace UI {

/**
 * @brief DBus signal callback
 * @param signal_name Name of the signal
 * @param parameters GVariant containing signal parameters
 */
using DBusSignalCallback = std::function<void(const std::string& signal_name, GVariant* parameters)>;

/**
 * @brief DBus property change callback
 * @param property_name Name of the changed property
 * @param value New value of the property
 */
using DBusPropertyCallback = std::function<void(const std::string& property_name, GVariant* value)>;

/**
 * @brief Base class for DBus integration
 * 
 * Provides easy access to DBus services without requiring widgets to know DBus API details.
 * Handles:
 * - Connection management (system/session bus)
 * - Proxy creation for service interfaces
 * - Signal subscription with callbacks
 * - Property monitoring
 * - Method calls (sync and async)
 * - Automatic reconnection on bus loss
 * 
 * Example usage:
 * 
 * class BatteryWidget : public PeriodicWidget, public DBusHelper {
 *   void Initialize() {
 *     ConnectToSystemBus();
 *     
 *     CreateProxy("org.freedesktop.UPower",
 *                "/org/freedesktop/UPower/devices/battery_BAT0",
 *                "org.freedesktop.UPower.Device");
 *     
 *     SubscribeToSignal("PropertiesChanged", [this](auto name, auto params) {
 *       UpdateBatteryInfo();
 *       MarkDirty();
 *     });
 *     
 *     MonitorProperty("Percentage", [this](auto name, auto value) {
 *       battery_percentage_ = GetDoubleFromVariant(value);
 *       MarkDirty();
 *     });
 *   }
 * };
 */
class DBusHelper {
public:
    DBusHelper();
    virtual ~DBusHelper();
    
    // Connection management
    bool ConnectToSystemBus();
    bool ConnectToSessionBus();
    bool IsConnected() const { return connection_ != nullptr; }
    void Disconnect();
    
    /**
     * @brief Create a proxy for a DBus service interface
     * @param bus_name The well-known bus name (e.g., "org.freedesktop.UPower")
     * @param object_path The object path (e.g., "/org/freedesktop/UPower")
     * @param interface_name The interface name (e.g., "org.freedesktop.UPower")
     * @return true if proxy was created successfully
     */
    bool CreateProxy(const std::string& bus_name,
                    const std::string& object_path,
                    const std::string& interface_name);
    
    /**
     * @brief Subscribe to a DBus signal
     * @param signal_name Name of the signal to subscribe to
     * @param callback Function to call when signal is received
     * @return Subscription ID (use to unsubscribe later)
     */
    guint SubscribeToSignal(const std::string& signal_name, DBusSignalCallback callback);
    
    /**
     * @brief Unsubscribe from a DBus signal
     * @param subscription_id ID returned from SubscribeToSignal
     */
    void UnsubscribeFromSignal(guint subscription_id);
    
    /**
     * @brief Monitor a DBus property for changes
     * @param property_name Name of the property to monitor
     * @param callback Function to call when property changes
     */
    void MonitorProperty(const std::string& property_name, DBusPropertyCallback callback);
    
    /**
     * @brief Stop monitoring a property
     */
    void StopMonitoringProperty(const std::string& property_name);
    
    /**
     * @brief Call a DBus method synchronously
     * @param method_name Name of the method to call
     * @param parameters GVariant containing method parameters (or nullptr)
     * @return GVariant containing return value (caller must free with g_variant_unref)
     */
    GVariant* CallMethod(const std::string& method_name, GVariant* parameters = nullptr);
    
    /**
     * @brief Call a DBus method asynchronously
     * @param method_name Name of the method to call
     * @param parameters GVariant containing method parameters (or nullptr)
     * @param callback Function to call when method completes
     */
    void CallMethodAsync(const std::string& method_name,
                        GVariant* parameters,
                        std::function<void(GVariant* result, GError* error)> callback);
    
    /**
     * @brief Get a property value
     * @param property_name Name of the property
     * @return GVariant containing property value (caller must free)
     */
    GVariant* GetProperty(const std::string& property_name);
    
    /**
     * @brief Set a property value
     * @param property_name Name of the property
     * @param value New value for the property
     * @return true if property was set successfully
     */
    bool SetProperty(const std::string& property_name, GVariant* value);
    
    /**
     * @brief Enumerate objects at a path (useful for finding devices)
     * @param object_path Path to enumerate (e.g., "/org/freedesktop/UPower/devices")
     * @return Vector of object paths
     */
    std::vector<std::string> EnumerateObjects(const std::string& object_path);
    
    // Helper methods to extract values from GVariant
    static bool GetBoolFromVariant(GVariant* variant);
    static int GetInt32FromVariant(GVariant* variant);
    static uint32_t GetUInt32FromVariant(GVariant* variant);
    static int64_t GetInt64FromVariant(GVariant* variant);
    static uint64_t GetUInt64FromVariant(GVariant* variant);
    static double GetDoubleFromVariant(GVariant* variant);
    static std::string GetStringFromVariant(GVariant* variant);
    static std::vector<std::string> GetStringArrayFromVariant(GVariant* variant);
    
    // Helper methods to create GVariant values
    static GVariant* CreateBoolVariant(bool value);
    static GVariant* CreateInt32Variant(int value);
    static GVariant* CreateUInt32Variant(uint32_t value);
    static GVariant* CreateInt64Variant(int64_t value);
    static GVariant* CreateUInt64Variant(uint64_t value);
    static GVariant* CreateDoubleVariant(double value);
    static GVariant* CreateStringVariant(const std::string& value);

protected:
    /**
     * @brief Called when connection is lost (override for reconnection logic)
     */
    virtual void OnConnectionLost() {}
    
    /**
     * @brief Called when successfully connected
     */
    virtual void OnConnected() {}
    
    /**
     * @brief Get the raw proxy (for advanced use)
     */
    GDBusProxy* GetProxy() const { return proxy_; }
    
    /**
     * @brief Get the raw connection (for advanced use)
     */
    GDBusConnection* GetConnection() const { return connection_; }

private:
    // Static callback for signal handling
    static void OnSignalReceived(GDBusConnection* connection,
                                const char* sender_name,
                                const char* object_path,
                                const char* interface_name,
                                const char* signal_name,
                                GVariant* parameters,
                                void* user_data);
    
    // Static callback for property changes
    static void OnPropertiesChanged(GDBusProxy* proxy,
                                   GVariant* changed_properties,
                                   const char* const* invalidated_properties,
                                   void* user_data);
    
    // Connection and proxy
    GDBusConnection* connection_;
    GDBusProxy* proxy_;
    
    // Signal subscriptions
    std::map<guint, DBusSignalCallback> signal_callbacks_;
    std::map<std::string, DBusPropertyCallback> property_callbacks_;
    guint properties_changed_signal_id_;
    
    // Proxy information (for reconnection)
    std::string bus_name_;
    std::string object_path_;
    std::string interface_name_;
    
    // Thread safety
    mutable std::mutex dbus_mutex_;
};

} // namespace UI
} // namespace Leviathan
