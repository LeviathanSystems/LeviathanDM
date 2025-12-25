#pragma once

#include "BaseWidget.hpp"
#include <string>
#include <memory>
#include <map>

namespace Leviathan {
namespace UI {

// Plugin metadata
struct PluginMetadata {
    std::string name;           // Plugin name (e.g., "ClockWidget")
    std::string version;        // Plugin version (e.g., "1.0.0")
    std::string author;         // Plugin author
    std::string description;    // What the plugin does
    int api_version;            // Widget API version this plugin targets
};

// Current widget API version
constexpr int WIDGET_API_VERSION = 1;

// Plugin interface - all plugin widgets must inherit from this
class WidgetPlugin : public Widget {
public:
    virtual ~WidgetPlugin() = default;
    
    // Get plugin metadata
    virtual PluginMetadata GetMetadata() const = 0;
    
    // Initialize plugin with configuration
    // config is a key-value map from YAML config
    virtual bool Initialize(const std::map<std::string, std::string>& config) = 0;
    
    // Update plugin data (called from background thread)
    // This is where you fetch data, update state, etc.
    virtual void Update() = 0;
    
    // Cleanup (called before unloading)
    virtual void Cleanup() {}
};

// Plugin factory function type
// Each plugin .so must export a function of this type
typedef WidgetPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(WidgetPlugin*);

// Plugin descriptor - returned by plugin .so files
struct PluginDescriptor {
    PluginMetadata metadata;
    CreatePluginFunc create;
    DestroyPluginFunc destroy;
};

// Export macros for plugin developers
// Use these in your plugin .cpp file:
//
// extern "C" {
//     EXPORT_PLUGIN_CREATE(MyWidget)
//     EXPORT_PLUGIN_DESTROY(MyWidget)
//     EXPORT_PLUGIN_METADATA(MyWidget)
// }

#define EXPORT_PLUGIN_CREATE(ClassName) \
    Leviathan::UI::WidgetPlugin* CreatePlugin() { \
        return new Leviathan::UI::ClassName(); \
    }

#define EXPORT_PLUGIN_DESTROY(ClassName) \
    void DestroyPlugin(Leviathan::UI::WidgetPlugin* plugin) { \
        delete plugin; \
    }

#define EXPORT_PLUGIN_METADATA(ClassName) \
    Leviathan::UI::PluginMetadata GetPluginMetadata() { \
        Leviathan::UI::ClassName temp; \
        return temp.GetMetadata(); \
    }

} // namespace UI
} // namespace Leviathan
