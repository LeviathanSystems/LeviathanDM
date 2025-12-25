#include "ui/DBusHelper.hpp"
#include "Logger.hpp"

#include <gio/gio.h>
#include <algorithm>

namespace Leviathan {
namespace UI {

DBusHelper::DBusHelper()
    : connection_(nullptr),
      proxy_(nullptr),
      properties_changed_signal_id_(0) {
}

DBusHelper::~DBusHelper() {
    Disconnect();
}

bool DBusHelper::ConnectToSystemBus() {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (connection_) {
        LOG_WARN("Already connected to DBus");
        return true;
    }
    
    GError* error = nullptr;
    connection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    
    if (error) {
        LOG_ERROR_FMT("Failed to connect to system bus: {}", error->message);
        g_error_free(error);
        return false;
    }
    
    LOG_INFO("Connected to system DBus");
    OnConnected();
    return true;
}

bool DBusHelper::ConnectToSessionBus() {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (connection_) {
        LOG_WARN("Already connected to DBus");
        return true;
    }
    
    GError* error = nullptr;
    connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    
    if (error) {
        LOG_ERROR_FMT("Failed to connect to session bus: {}", error->message);
        g_error_free(error);
        return false;
    }
    
    LOG_INFO("Connected to session DBus");
    OnConnected();
    return true;
}

void DBusHelper::Disconnect() {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    // Unsubscribe from all signals
    for (const auto& [subscription_id, callback] : signal_callbacks_) {
        if (connection_) {
            g_dbus_connection_signal_unsubscribe(connection_, subscription_id);
        }
    }
    signal_callbacks_.clear();
    
    if (properties_changed_signal_id_ > 0 && proxy_) {
        g_signal_handler_disconnect(proxy_, properties_changed_signal_id_);
        properties_changed_signal_id_ = 0;
    }
    
    if (proxy_) {
        g_object_unref(proxy_);
        proxy_ = nullptr;
    }
    
    if (connection_) {
        g_object_unref(connection_);
        connection_ = nullptr;
    }
    
    LOG_DEBUG("Disconnected from DBus");
}

bool DBusHelper::CreateProxy(const std::string& bus_name,
                             const std::string& object_path,
                             const std::string& interface_name) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (!connection_) {
        LOG_ERROR("Not connected to DBus - call ConnectToSystemBus() or ConnectToSessionBus() first");
        return false;
    }
    
    if (proxy_) {
        g_object_unref(proxy_);
        proxy_ = nullptr;
    }
    
    // Store proxy info for reconnection
    bus_name_ = bus_name;
    object_path_ = object_path;
    interface_name_ = interface_name;
    
    GError* error = nullptr;
    proxy_ = g_dbus_proxy_new_sync(
        connection_,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
        nullptr,  // GDBusInterfaceInfo
        bus_name.c_str(),
        object_path.c_str(),
        interface_name.c_str(),
        nullptr,  // GCancellable
        &error
    );
    
    if (error) {
        LOG_ERROR_FMT("Failed to create DBus proxy for {}: {}", bus_name, error->message);
        g_error_free(error);
        return false;
    }
    
    // Subscribe to property changes
    properties_changed_signal_id_ = g_signal_connect(
        proxy_,
        "g-properties-changed",
        G_CALLBACK(OnPropertiesChanged),
        this
    );
    
    LOG_DEBUG_FMT("Created DBus proxy: {} {} {}", bus_name, object_path, interface_name);
    return true;
}

guint DBusHelper::SubscribeToSignal(const std::string& signal_name, DBusSignalCallback callback) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (!connection_) {
        LOG_ERROR("Not connected to DBus");
        return 0;
    }
    
    // Subscribe to signal
    guint subscription_id = g_dbus_connection_signal_subscribe(
        connection_,
        bus_name_.empty() ? nullptr : bus_name_.c_str(),  // sender
        interface_name_.c_str(),  // interface
        signal_name.c_str(),      // member (signal name)
        object_path_.empty() ? nullptr : object_path_.c_str(),  // object path
        nullptr,                  // arg0 (match string)
        G_DBUS_SIGNAL_FLAGS_NONE,
        OnSignalReceived,
        this,
        nullptr  // user_data_free_func
    );
    
    if (subscription_id > 0) {
        signal_callbacks_[subscription_id] = callback;
        LOG_DEBUG_FMT("Subscribed to signal '{}' (id: {})", signal_name, subscription_id);
    } else {
        LOG_ERROR_FMT("Failed to subscribe to signal '{}'", signal_name);
    }
    
    return subscription_id;
}

void DBusHelper::UnsubscribeFromSignal(guint subscription_id) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (connection_ && subscription_id > 0) {
        g_dbus_connection_signal_unsubscribe(connection_, subscription_id);
        signal_callbacks_.erase(subscription_id);
        LOG_DEBUG_FMT("Unsubscribed from signal (id: {})", subscription_id);
    }
}

void DBusHelper::MonitorProperty(const std::string& property_name, DBusPropertyCallback callback) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    property_callbacks_[property_name] = callback;
    LOG_DEBUG_FMT("Monitoring property '{}'", property_name);
}

void DBusHelper::StopMonitoringProperty(const std::string& property_name) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    property_callbacks_.erase(property_name);
    LOG_DEBUG_FMT("Stopped monitoring property '{}'", property_name);
}

GVariant* DBusHelper::CallMethod(const std::string& method_name, GVariant* parameters) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (!proxy_) {
        LOG_ERROR("No proxy created - call CreateProxy() first");
        return nullptr;
    }
    
    GError* error = nullptr;
    GVariant* result = g_dbus_proxy_call_sync(
        proxy_,
        method_name.c_str(),
        parameters,
        G_DBUS_CALL_FLAGS_NONE,
        -1,  // timeout (default)
        nullptr,  // cancellable
        &error
    );
    
    if (error) {
        LOG_ERROR_FMT("Failed to call method '{}': {}", method_name, error->message);
        g_error_free(error);
        return nullptr;
    }
    
    return result;
}

void DBusHelper::CallMethodAsync(const std::string& method_name,
                                GVariant* parameters,
                                std::function<void(GVariant*, GError*)> callback) {
    if (!proxy_) {
        LOG_ERROR("No proxy created - call CreateProxy() first");
        if (callback) {
            callback(nullptr, nullptr);
        }
        return;
    }
    
    // Create callback data
    auto* cb_data = new std::function<void(GVariant*, GError*)>(callback);
    
    g_dbus_proxy_call(
        proxy_,
        method_name.c_str(),
        parameters,
        G_DBUS_CALL_FLAGS_NONE,
        -1,  // timeout
        nullptr,  // cancellable
        [](GObject* source, GAsyncResult* res, gpointer user_data) {
            auto* callback = static_cast<std::function<void(GVariant*, GError*)>*>(user_data);
            GError* error = nullptr;
            GVariant* result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
            
            if (*callback) {
                (*callback)(result, error);
            }
            
            if (result) {
                g_variant_unref(result);
            }
            if (error) {
                g_error_free(error);
            }
            
            delete callback;
        },
        cb_data
    );
}

GVariant* DBusHelper::GetProperty(const std::string& property_name) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (!proxy_) {
        LOG_ERROR("No proxy created - call CreateProxy() first");
        return nullptr;
    }
    
    GVariant* value = g_dbus_proxy_get_cached_property(proxy_, property_name.c_str());
    
    if (!value) {
        LOG_DEBUG_FMT("Property '{}' not in cache, fetching...", property_name);
        // Property not in cache, fetch it
        GError* error = nullptr;
        GVariant* result = g_dbus_proxy_call_sync(
            proxy_,
            "org.freedesktop.DBus.Properties.Get",
            g_variant_new("(ss)", interface_name_.c_str(), property_name.c_str()),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            &error
        );
        
        if (error) {
            LOG_ERROR_FMT("Failed to get property '{}': {}", property_name, error->message);
            g_error_free(error);
            return nullptr;
        }
        
        if (result) {
            g_variant_get(result, "(v)", &value);
            g_variant_unref(result);
        }
    }
    
    return value;
}

bool DBusHelper::SetProperty(const std::string& property_name, GVariant* value) {
    std::lock_guard<std::mutex> lock(dbus_mutex_);
    
    if (!proxy_) {
        LOG_ERROR("No proxy created - call CreateProxy() first");
        return false;
    }
    
    GError* error = nullptr;
    GVariant* result = g_dbus_proxy_call_sync(
        proxy_,
        "org.freedesktop.DBus.Properties.Set",
        g_variant_new("(ssv)", interface_name_.c_str(), property_name.c_str(), value),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (error) {
        LOG_ERROR_FMT("Failed to set property '{}': {}", property_name, error->message);
        g_error_free(error);
        return false;
    }
    
    if (result) {
        g_variant_unref(result);
    }
    
    return true;
}

std::vector<std::string> DBusHelper::EnumerateObjects(const std::string& object_path) {
    std::vector<std::string> objects;
    
    if (!connection_) {
        LOG_ERROR("Not connected to DBus");
        return objects;
    }
    
    // This is a common pattern for enumerating objects
    // Different services might use different methods, so this is a basic implementation
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        bus_name_.c_str(),
        object_path.c_str(),
        "org.freedesktop.DBus.Introspectable",
        "Introspect",
        nullptr,
        G_VARIANT_TYPE("(s)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (error) {
        LOG_DEBUG_FMT("Failed to introspect {}: {}", object_path, error->message);
        g_error_free(error);
        return objects;
    }
    
    if (result) {
        const char* xml_data = nullptr;
        g_variant_get(result, "(&s)", &xml_data);
        
        // Parse XML to extract child nodes (simplified)
        // A full implementation would use proper XML parsing
        std::string xml(xml_data);
        size_t pos = 0;
        while ((pos = xml.find("<node name=\"", pos)) != std::string::npos) {
            pos += 12;  // strlen("<node name=\"")
            size_t end = xml.find("\"", pos);
            if (end != std::string::npos) {
                std::string node_name = xml.substr(pos, end - pos);
                objects.push_back(object_path + "/" + node_name);
            }
            pos = end;
        }
        
        g_variant_unref(result);
    }
    
    return objects;
}

// Static signal callback
void DBusHelper::OnSignalReceived(GDBusConnection* connection,
                                  const char* sender_name,
                                  const char* object_path,
                                  const char* interface_name,
                                  const char* signal_name,
                                  GVariant* parameters,
                                  void* user_data) {
    auto* helper = static_cast<DBusHelper*>(user_data);
    std::lock_guard<std::mutex> lock(helper->dbus_mutex_);
    
    // Find and call appropriate callback
    for (const auto& [subscription_id, callback] : helper->signal_callbacks_) {
        if (callback) {
            callback(signal_name, parameters);
        }
    }
}

// Static property change callback
void DBusHelper::OnPropertiesChanged(GDBusProxy* proxy,
                                    GVariant* changed_properties,
                                    const char* const* invalidated_properties,
                                    void* user_data) {
    auto* helper = static_cast<DBusHelper*>(user_data);
    std::lock_guard<std::mutex> lock(helper->dbus_mutex_);
    
    // Iterate over changed properties
    GVariantIter iter;
    const char* property_name;
    GVariant* property_value;
    
    g_variant_iter_init(&iter, changed_properties);
    while (g_variant_iter_next(&iter, "{&sv}", &property_name, &property_value)) {
        // Check if we're monitoring this property
        auto it = helper->property_callbacks_.find(property_name);
        if (it != helper->property_callbacks_.end() && it->second) {
            it->second(property_name, property_value);
        }
        g_variant_unref(property_value);
    }
}

// Helper methods to extract values from GVariant
bool DBusHelper::GetBoolFromVariant(GVariant* variant) {
    if (!variant) return false;
    return g_variant_get_boolean(variant);
}

int DBusHelper::GetInt32FromVariant(GVariant* variant) {
    if (!variant) return 0;
    return g_variant_get_int32(variant);
}

uint32_t DBusHelper::GetUInt32FromVariant(GVariant* variant) {
    if (!variant) return 0;
    return g_variant_get_uint32(variant);
}

int64_t DBusHelper::GetInt64FromVariant(GVariant* variant) {
    if (!variant) return 0;
    return g_variant_get_int64(variant);
}

uint64_t DBusHelper::GetUInt64FromVariant(GVariant* variant) {
    if (!variant) return 0;
    return g_variant_get_uint64(variant);
}

double DBusHelper::GetDoubleFromVariant(GVariant* variant) {
    if (!variant) return 0.0;
    return g_variant_get_double(variant);
}

std::string DBusHelper::GetStringFromVariant(GVariant* variant) {
    if (!variant) return "";
    const char* str = g_variant_get_string(variant, nullptr);
    return str ? std::string(str) : "";
}

std::vector<std::string> DBusHelper::GetStringArrayFromVariant(GVariant* variant) {
    std::vector<std::string> result;
    if (!variant) return result;
    
    gsize length = 0;
    const gchar** strv = g_variant_get_strv(variant, &length);
    
    for (gsize i = 0; i < length; i++) {
        result.push_back(strv[i]);
    }
    
    g_free(strv);
    return result;
}

// Helper methods to create GVariant values
GVariant* DBusHelper::CreateBoolVariant(bool value) {
    return g_variant_new_boolean(value);
}

GVariant* DBusHelper::CreateInt32Variant(int value) {
    return g_variant_new_int32(value);
}

GVariant* DBusHelper::CreateUInt32Variant(uint32_t value) {
    return g_variant_new_uint32(value);
}

GVariant* DBusHelper::CreateInt64Variant(int64_t value) {
    return g_variant_new_int64(value);
}

GVariant* DBusHelper::CreateUInt64Variant(uint64_t value) {
    return g_variant_new_uint64(value);
}

GVariant* DBusHelper::CreateDoubleVariant(double value) {
    return g_variant_new_double(value);
}

GVariant* DBusHelper::CreateStringVariant(const std::string& value) {
    return g_variant_new_string(value.c_str());
}

} // namespace UI
} // namespace Leviathan
