#pragma once

#include "ui/WidgetPlugin.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <dlfcn.h>

namespace Leviathan {
namespace UI {

// Loaded plugin information
struct LoadedPlugin {
    void* handle;                           // dlopen handle
    PluginDescriptor descriptor;            // Plugin descriptor
    std::string path;                       // Path to .so file
    std::vector<WidgetPlugin*> instances;   // Active instances of this plugin
};

// Plugin manager - handles loading and managing widget plugins
class WidgetPluginManager {
public:
    static WidgetPluginManager& Instance() {
        static WidgetPluginManager instance;
        return instance;
    }
    
    // Load a plugin from .so file
    bool LoadPlugin(const std::string& plugin_path);
    
    // Unload a plugin
    void UnloadPlugin(const std::string& plugin_name);
    
    // Unload all plugins
    void UnloadAll();
    
    // Create an instance of a plugin widget
    std::shared_ptr<WidgetPlugin> CreatePluginWidget(
        const std::string& plugin_name,
        const std::map<std::string, std::string>& config);
    
    // Discover and load all plugins from a directory
    void DiscoverPlugins(const std::string& plugin_dir);
    
    // Get list of loaded plugin names
    std::vector<std::string> GetLoadedPlugins() const;
    
    // Check if a plugin is loaded
    bool IsPluginLoaded(const std::string& plugin_name) const;
    
    // Get plugin metadata
    PluginMetadata GetPluginMetadata(const std::string& plugin_name) const;

private:
    WidgetPluginManager() = default;
    ~WidgetPluginManager();
    
    // Disable copy and move
    WidgetPluginManager(const WidgetPluginManager&) = delete;
    WidgetPluginManager& operator=(const WidgetPluginManager&) = delete;
    
    // Validate plugin API version
    bool ValidatePlugin(const PluginDescriptor& descriptor) const;
    
    // Map of plugin name -> loaded plugin info
    std::map<std::string, LoadedPlugin> plugins_;
};

// Global convenience accessor
inline WidgetPluginManager& PluginManager() {
    return WidgetPluginManager::Instance();
}

} // namespace UI
} // namespace Leviathan
