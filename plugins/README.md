# LeviathanDM Widget Plugins

This directory contains the widget plugin system for LeviathanDM.

## Documentation

- **[PLUGIN_API.md](PLUGIN_API.md)** - Quick reference for the Plugin API
- **[PLUGIN_DEV_GUIDE.md](PLUGIN_DEV_GUIDE.md)** - Complete development guide

## Example Plugins

- **[clock-widget/](clock-widget/)** - Simple clock display widget
- **[taglist-widget/](taglist-widget/)** - Tag list with app counts (uses compositor state)

## Quick Start

### 1. Include the Plugin API

```cpp
#include "ui/PluginAPI.hpp"
```

This is the **ONLY** header you need to include.

### 2. Create Your Widget

```cpp
class MyWidget : public Leviathan::UI::WidgetPlugin {
public:
    void Update() override {
        // Query compositor state
        auto* state = Leviathan::UI::GetCompositorState();
        // ...
    }
    
    void Render(cairo_t* cr) override {
        // Draw with Cairo
    }
};
```

### 3. Export Plugin Functions

```cpp
extern "C" {
    EXPORT_PLUGIN_CREATE(MyWidget)
    EXPORT_PLUGIN_DESTROY(MyWidget)
    EXPORT_PLUGIN_METADATA(MyWidget)
}
```

### 4. Build

```bash
mkdir build && cd build
cmake ..
make
```

### 5. Install

```bash
# User install
cp my-widget.so ~/.config/leviathan/plugins/

# System install
sudo make install
```

## Building All Plugins

```bash
./build-all.sh
```

## Plugin API Features

✅ **Single Header** - Just include `ui/PluginAPI.hpp`  
✅ **No wlroots** - Plugins don't need compositor internals  
✅ **Compositor State** - Query screens, tags, clients  
✅ **Thread Safe** - Background updates don't block compositor  
✅ **Easy Export** - Simple macros for plugin functions  

## Helper Functions

Access compositor state using helper functions:

```cpp
// Tags
std::string name = Leviathan::UI::Plugin::GetTagName(tag);
int clients = Leviathan::UI::Plugin::GetTagClientCount(tag);

// Clients
std::string title = Leviathan::UI::Plugin::GetClientTitle(client);
std::string app = Leviathan::UI::Plugin::GetClientAppId(client);

// Screens
std::string name = Leviathan::UI::Plugin::GetScreenName(screen);
int width = Leviathan::UI::Plugin::GetScreenWidth(screen);
```

See [PLUGIN_API.md](PLUGIN_API.md) for full reference.
