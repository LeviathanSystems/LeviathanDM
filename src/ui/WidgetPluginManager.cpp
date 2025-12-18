#include "ui/WidgetPluginManager.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <algorithm>
#include <dlfcn.h>

namespace Leviathan {
namespace UI {

WidgetPluginManager::~WidgetPluginManager() {
    UnloadAll();
}

bool WidgetPluginManager::LoadPlugin(const std::string& plugin_path) {
    LOG_INFO("Loading widget plugin: {}", plugin_path);
    
    // Open the shared library
    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        LOG_ERROR("Failed to load plugin {}: {}", plugin_path, dlerror());
        return false;
    }
    
    // Clear any existing errors
    dlerror();
    
    // Load the required functions
    auto create_func = reinterpret_cast<CreatePluginFunc>(dlsym(handle, "CreatePlugin"));
    auto destroy_func = reinterpret_cast<DestroyPluginFunc>(dlsym(handle, "DestroyPlugin"));
    auto metadata_func = reinterpret_cast<PluginMetadata(*)()>(dlsym(handle, "GetPluginMetadata"));
    
    const char* dlsym_error = dlerror();
    if (dlsym_error || !create_func || !destroy_func || !metadata_func) {
        LOG_ERROR("Failed to load plugin symbols from {}: {}", 
                  plugin_path, dlsym_error ? dlsym_error : "Missing required functions");
        dlclose(handle);
        return false;
    }
    
    // Get plugin metadata
    PluginMetadata metadata = metadata_func();
    
    // Create descriptor
    PluginDescriptor descriptor;
    descriptor.metadata = metadata;
    descriptor.create = create_func;
    descriptor.destroy = destroy_func;
    
    // Validate API version
    if (!ValidatePlugin(descriptor)) {
        LOG_ERROR("Plugin {} has incompatible API version {}, expected {}",
                  metadata.name, metadata.api_version, WIDGET_API_VERSION);
        dlclose(handle);
        return false;
    }
    
    // Check if plugin with same name already loaded
    if (IsPluginLoaded(metadata.name)) {
        LOG_WARN("Plugin {} already loaded, unloading old version", metadata.name);
        UnloadPlugin(metadata.name);
    }
    
    // Store plugin info
    LoadedPlugin loaded;
    loaded.handle = handle;
    loaded.descriptor = descriptor;
    loaded.path = plugin_path;
    
    plugins_[metadata.name] = loaded;
    
    LOG_INFO("Successfully loaded plugin: {} v{} by {}",
             metadata.name, metadata.version, metadata.author);
    LOG_DEBUG("Plugin description: {}", metadata.description);
    
    return true;
}

void WidgetPluginManager::UnloadPlugin(const std::string& plugin_name) {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        LOG_WARN("Attempted to unload plugin {} which is not loaded", plugin_name);
        return;
    }
    
    LOG_INFO("Unloading plugin: {}", plugin_name);
    
    LoadedPlugin& loaded = it->second;
    
    // Destroy all instances
    for (auto* instance : loaded.instances) {
        instance->Cleanup();
        loaded.descriptor.destroy(instance);
    }
    loaded.instances.clear();
    
    // Close the shared library
    if (loaded.handle) {
        dlclose(loaded.handle);
    }
    
    plugins_.erase(it);
}

void WidgetPluginManager::UnloadAll() {
    LOG_INFO("Unloading all widget plugins");
    
    auto plugin_names = GetLoadedPlugins();
    for (const auto& name : plugin_names) {
        UnloadPlugin(name);
    }
}

std::shared_ptr<WidgetPlugin> WidgetPluginManager::CreatePluginWidget(
    const std::string& plugin_name,
    const std::map<std::string, std::string>& config) {
    
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        LOG_ERROR("Cannot create widget: plugin {} not loaded", plugin_name);
        return nullptr;
    }
    
    LoadedPlugin& loaded = it->second;
    
    // Create instance
    WidgetPlugin* instance = loaded.descriptor.create();
    if (!instance) {
        LOG_ERROR("Plugin {} create function returned nullptr", plugin_name);
        return nullptr;
    }
    
    // Initialize with config
    if (!instance->Initialize(config)) {
        LOG_ERROR("Failed to initialize plugin widget {}", plugin_name);
        loaded.descriptor.destroy(instance);
        return nullptr;
    }
    
    // Track instance
    loaded.instances.push_back(instance);
    
    LOG_DEBUG("Created instance of plugin widget: {}", plugin_name);
    
    // Return as shared_ptr with custom deleter
    return std::shared_ptr<WidgetPlugin>(instance, [this, plugin_name](WidgetPlugin* ptr) {
        // Custom deleter: remove from instances and destroy
        auto it = plugins_.find(plugin_name);
        if (it != plugins_.end()) {
            auto& instances = it->second.instances;
            auto remove_it = std::remove(instances.begin(), instances.end(), ptr);
            instances.erase(remove_it, instances.end());
            it->second.descriptor.destroy(ptr);
        }
    });
}

void WidgetPluginManager::DiscoverPlugins(const std::string& plugin_dir) {
    LOG_INFO("Discovering widget plugins in: {}", plugin_dir);
    
    if (!std::filesystem::exists(plugin_dir)) {
        LOG_WARN("Plugin directory does not exist: {}", plugin_dir);
        return;
    }
    
    int loaded_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (!entry.is_regular_file()) continue;
        
        std::string path = entry.path().string();
        std::string ext = entry.path().extension().string();
        
        // Only load .so files
        if (ext != ".so") continue;
        
        if (LoadPlugin(path)) {
            loaded_count++;
        }
    }
    
    LOG_INFO("Discovered and loaded {} widget plugins from {}", loaded_count, plugin_dir);
}

std::vector<std::string> WidgetPluginManager::GetLoadedPlugins() const {
    std::vector<std::string> names;
    names.reserve(plugins_.size());
    
    for (const auto& [name, _] : plugins_) {
        names.push_back(name);
    }
    
    return names;
}

bool WidgetPluginManager::IsPluginLoaded(const std::string& plugin_name) const {
    return plugins_.find(plugin_name) != plugins_.end();
}

PluginMetadata WidgetPluginManager::GetPluginMetadata(const std::string& plugin_name) const {
    auto it = plugins_.find(plugin_name);
    if (it != plugins_.end()) {
        return it->second.descriptor.metadata;
    }
    return PluginMetadata{};
}

bool WidgetPluginManager::ValidatePlugin(const PluginDescriptor& descriptor) const {
    // Check API version compatibility
    if (descriptor.metadata.api_version != WIDGET_API_VERSION) {
        return false;
    }
    
    // Ensure required functions are present
    if (!descriptor.create || !descriptor.destroy) {
        return false;
    }
    
    return true;
}

} // namespace UI
} // namespace Leviathan
