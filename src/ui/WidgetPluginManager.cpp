#include "ui/WidgetPluginManager.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <dlfcn.h>

namespace Leviathan {
namespace UI {

WidgetPluginManager::~WidgetPluginManager() {
    UnloadAll();
}

bool WidgetPluginManager::LoadPlugin(const std::string& plugin_path) {
    LOG_INFO_FMT("Loading widget plugin: {}", plugin_path);
    
    // Measure memory BEFORE loading plugin
    size_t baseline_rss, baseline_virtual;
    GetProcessMemory(baseline_rss, baseline_virtual);
    
    // Open the shared library
    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        LOG_ERROR_FMT("Failed to load plugin {}: {}", plugin_path, dlerror());
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
        LOG_ERROR_FMT("Failed to load plugin symbols from {}: {}", 
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
        LOG_ERROR_FMT("Plugin {} has incompatible API version {}, expected {}",
                  metadata.name, metadata.api_version, WIDGET_API_VERSION);
        dlclose(handle);
        return false;
    }
    
    // Check if plugin with same name already loaded
    if (IsPluginLoaded(metadata.name)) {
        LOG_WARN_FMT("Plugin {} already loaded, unloading old version", metadata.name);
        UnloadPlugin(metadata.name);
    }
    
    // Store plugin info
    LoadedPlugin loaded;
    loaded.handle = handle;
    loaded.descriptor = descriptor;
    loaded.path = plugin_path;
    loaded.baseline_rss = baseline_rss;
    loaded.baseline_virtual = baseline_virtual;
    
    plugins_[metadata.name] = loaded;
    
    // Measure memory AFTER loading to see the delta
    size_t after_rss, after_virtual;
    GetProcessMemory(after_rss, after_virtual);
    
    size_t delta_rss = after_rss > baseline_rss ? after_rss - baseline_rss : 0;
    size_t delta_virtual = after_virtual > baseline_virtual ? after_virtual - baseline_virtual : 0;
    
    LOG_INFO_FMT("Successfully loaded plugin: {} v{} by {}",
             metadata.name, metadata.version, metadata.author);
    LOG_DEBUG_FMT("Plugin description: {}", metadata.description);
    LOG_DEBUG_FMT("Memory delta - RSS: {} KB, Virtual: {} KB", 
             delta_rss / 1024, delta_virtual / 1024);
    
    return true;
}

void WidgetPluginManager::UnloadPlugin(const std::string& plugin_name) {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        LOG_WARN_FMT("Attempted to unload plugin {} which is not loaded", plugin_name);
        return;
    }
    
    LOG_INFO_FMT("Unloading plugin: {}", plugin_name);
    
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
        LOG_ERROR_FMT("Cannot create widget: plugin {} not loaded", plugin_name);
        return nullptr;
    }
    
    LoadedPlugin& loaded = it->second;
    
    // Create instance
    WidgetPlugin* instance = loaded.descriptor.create();
    if (!instance) {
        LOG_ERROR_FMT("Plugin {} create function returned nullptr", plugin_name);
        return nullptr;
    }
    
    // Initialize with config
    if (!instance->Initialize(config)) {
        LOG_ERROR_FMT("Failed to initialize plugin widget {}", plugin_name);
        loaded.descriptor.destroy(instance);
        return nullptr;
    }
    
    // Track instance
    loaded.instances.push_back(instance);
    
    // Get metadata from the instance
    auto metadata = instance->GetMetadata();
    LOG_INFO_FMT("Created instance of plugin widget: {} v{}", plugin_name, metadata.version);
    
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
    LOG_INFO_FMT("Discovering widget plugins in: {}", plugin_dir);
    
    if (!std::filesystem::exists(plugin_dir)) {
        LOG_WARN_FMT("Plugin directory does not exist: {}", plugin_dir);
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
    
    LOG_INFO_FMT("Discovered and loaded {} widget plugins from {}", loaded_count, plugin_dir);
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

void WidgetPluginManager::GetProcessMemory(size_t& rss_bytes, size_t& virtual_bytes) const {
    rss_bytes = 0;
    virtual_bytes = 0;
    
    // Read /proc/self/statm on Linux
    std::ifstream statm("/proc/self/statm");
    if (!statm.is_open()) {
        return;
    }
    
    // statm format: size resident shared text lib data dt
    // size = virtual memory (pages)
    // resident = RSS (pages)
    long page_size = sysconf(_SC_PAGESIZE);
    
    size_t vsize_pages, rss_pages;
    statm >> vsize_pages >> rss_pages;
    
    virtual_bytes = vsize_pages * page_size;
    rss_bytes = rss_pages * page_size;
}

WidgetPluginManager::PluginMemoryStats 
WidgetPluginManager::GetPluginMemoryStats(const std::string& plugin_name) const {
    PluginMemoryStats stats{0, 0, 0};
    
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        return stats;
    }
    
    const LoadedPlugin& plugin = it->second;
    
    // Get current process memory
    size_t current_rss, current_virtual;
    GetProcessMemory(current_rss, current_virtual);
    
    // Calculate plugin memory as delta from baseline + proportional instance memory
    size_t base_plugin_rss = current_rss > plugin.baseline_rss ? 
                             current_rss - plugin.baseline_rss : 0;
    size_t base_plugin_virtual = current_virtual > plugin.baseline_virtual ? 
                                 current_virtual - plugin.baseline_virtual : 0;
    
    // Count instances
    stats.instance_count = plugin.instances.size();
    
    // Base memory is from the .so file loading
    // Add estimated instance memory (rough estimate: 64KB per instance for widgets)
    const size_t ESTIMATED_INSTANCE_OVERHEAD = 64 * 1024; // 64 KB per instance
    
    stats.rss_bytes = base_plugin_rss + (stats.instance_count * ESTIMATED_INSTANCE_OVERHEAD);
    stats.virtual_bytes = base_plugin_virtual + (stats.instance_count * ESTIMATED_INSTANCE_OVERHEAD);
    
    return stats;
}

std::map<std::string, WidgetPluginManager::PluginMemoryStats>
WidgetPluginManager::GetAllPluginMemoryStats() const {
    std::map<std::string, PluginMemoryStats> all_stats;
    
    for (const auto& [name, _] : plugins_) {
        all_stats[name] = GetPluginMemoryStats(name);
    }
    
    return all_stats;
}

} // namespace UI
} // namespace Leviathan
