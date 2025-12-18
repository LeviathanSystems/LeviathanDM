# Widget Plugin Development Guide

## Overview

LeviathanDM supports dynamically loaded widget plugins that can extend the status bar functionality. Plugins are compiled as shared libraries (.so files) and loaded at runtime.

## Creating a Plugin

### 1. Plugin Structure

```cpp
#include "ui/WidgetPlugin.hpp"

class MyWidget : public WidgetPlugin {
public:
    // Get plugin metadata
    PluginMetadata GetMetadata() const override {
        return PluginMetadata{
            .name = "MyWidget",
            .version = "1.0.0",
            .author = "Your Name",
            .description = "What your plugin does",
            .api_version = WIDGET_API_VERSION
        };
    }
    
    // Initialize with config from YAML
    bool Initialize(const std::map<std::string, std::string>& config) override {
        // Parse config options
        // Start background threads if needed
        return true;
    }
    
    // Update data (called from background thread)
    void Update() override {
        // Fetch data, update state
        // Mark widget as dirty when data changes
    }
    
    // Calculate size based on content
    void CalculateSize(int available_width, int available_height) override {
        // Measure your content
        // Set width_ and height_
    }
    
    // Render to Cairo context
    void Render(cairo_t* cr) override {
        // Draw your widget
    }
    
    // Cleanup before unload
    void Cleanup() override {
        // Stop threads, free resources
    }
};

// Export required functions
extern "C" {
    EXPORT_PLUGIN_CREATE(MyWidget)
    EXPORT_PLUGIN_DESTROY(MyWidget)
    EXPORT_PLUGIN_METADATA(MyWidget)
}
```

### 2. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyWidget)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# Include LeviathanDM headers
include_directories(
    /path/to/LeviathanDM/include
    ${CAIRO_INCLUDE_DIRS}
)

# Build plugin
add_library(my-widget SHARED
    MyWidget.cpp
)

target_link_libraries(my-widget
    ${CAIRO_LIBRARIES}
    pthread  # If using threads
)

set_target_properties(my-widget PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)

install(TARGETS my-widget
    LIBRARY DESTINATION lib/leviathan/plugins
)
```

### 3. Build Your Plugin

```bash
cd plugins/my-widget
mkdir build && cd build
cmake ..
make
sudo make install  # Installs to /usr/local/lib/leviathan/plugins
```

## Thread Safety

### Background Thread Updates

Plugins can safely update data from background threads:

```cpp
void MyWidget::Initialize(const std::map<std::string, std::string>& config) {
    // Start background update thread
    update_thread_ = std::thread([this]() {
        while (running_) {
            // Fetch data (network, file, etc.)
            std::string data = FetchData();
            
            // Update widget (thread-safe - uses mutex_)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                data_ = data;
                dirty_ = true;  // Mark for re-render
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
    return true;
}
```

### Main Thread Rendering

Rendering always happens on the main Wayland thread:

```cpp
void MyWidget::Render(cairo_t* cr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Safe to access data_ - mutex is locked
    cairo_show_text(cr, data_.c_str());
}
```

## Configuration

Plugins receive configuration from the YAML file:

### YAML Config

```yaml
status_bar:
  widgets:
    - type: plugin
      name: MyWidget
      config:
        option1: "value1"
        option2: "value2"
        update_interval: "30"
```

### Parse in Plugin

```cpp
bool MyWidget::Initialize(const std::map<std::string, std::string>& config) {
    auto it = config.find("option1");
    if (it != config.end()) {
        option1_ = it->second;
    }
    
    auto interval_it = config.find("update_interval");
    if (interval_it != config.end()) {
        update_interval_ = std::stoi(interval_it->second);
    }
    
    return true;
}
```

## Example Plugins

### Clock Widget

See `plugins/clock-widget/` for a complete example that:
- Updates time every second in background thread
- Configurable format and font size
- Thread-safe updates
- Proper cleanup

### Building the Example

```bash
cd plugins/clock-widget
mkdir build && cd build
cmake -DCMAKE_SOURCE_DIR=$(pwd)/../.. ..
make
```

This creates `clock-widget.so` that can be loaded by LeviathanDM.

## Plugin API Reference

### Required Functions

Every plugin must export these C functions:

- `WidgetPlugin* CreatePlugin()` - Create instance
- `void DestroyPlugin(WidgetPlugin*)` - Destroy instance
- `PluginMetadata GetPluginMetadata()` - Get metadata

Use the provided macros:

```cpp
extern "C" {
    EXPORT_PLUGIN_CREATE(MyWidget)
    EXPORT_PLUGIN_DESTROY(MyWidget)
    EXPORT_PLUGIN_METADATA(MyWidget)
}
```

### Base Widget Members

Plugins inherit these from `Widget` base class:

- `int x_, y_` - Position (set by parent)
- `int width_, height_` - Size (set by CalculateSize)
- `bool visible_` - Visibility flag
- `bool dirty_` - Needs redraw flag
- `std::mutex mutex_` - Thread safety lock

### Helper Methods

- `void MarkDirty()` - Mark widget for re-render
- `bool IsDirty()` - Check if needs re-render
- `void SetVisible(bool)` - Show/hide widget
- `bool IsVisible()` - Check visibility

## Plugin Discovery

LeviathanDM searches for plugins in:

1. `~/.config/leviathan/plugins/`
2. `/usr/local/lib/leviathan/plugins/`
3. `/usr/lib/leviathan/plugins/`

Place your `.so` file in any of these locations.

## Debugging Plugins

Enable debug logging in LeviathanDM:

```cpp
LOG_DEBUG("My plugin: {}", some_value);
```

Check logs:
```bash
tail -f ~/.local/share/leviathan/leviathan.log
```

## Best Practices

✅ **Do:**
- Always lock `mutex_` when accessing member variables
- Mark widget `dirty_` when data changes
- Clean up threads in `Cleanup()`
- Validate config in `Initialize()`
- Handle missing config gracefully

❌ **Don't:**
- Block the main thread in `Render()`
- Access member variables without locking
- Leak resources (threads, memory, file handles)
- Assume config values exist
- Throw exceptions (return false instead)

## Troubleshooting

**Plugin not loading?**
- Check logs for dlopen errors
- Verify plugin exports required functions
- Check API version matches

**Crashes?**
- Ensure thread safety (lock mutex_)
- Check for nullptr before using pointers
- Validate config input

**Not rendering?**
- Call `MarkDirty()` when data changes
- Check if `visible_` is true
- Verify CalculateSize() sets width/height

## Example Plugin Ideas

- **CPU Monitor** - Show CPU usage graph
- **Memory Monitor** - Display RAM usage
- **Network Monitor** - Show network speed
- **Weather Widget** - Fetch and display weather
- **Music Player** - Show now playing info
- **Battery Widget** - Display battery status
- **Notification Counter** - Show unread count
- **Custom Command** - Run script and show output
