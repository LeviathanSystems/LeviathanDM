# Plugin API Reference

## Single Header Include

```cpp
#include "ui/PluginAPI.hpp"
```

This is the **ONLY** header plugins need to include. It provides:
- `WidgetPlugin` base class
- `CompositorState` interface
- Helper functions for querying compositor objects
- Export macros (`EXPORT_PLUGIN_CREATE`, etc.)

## Why This API Exists

The actual `Tag`, `Client`, and `Screen` classes are defined in headers that include wlroots (the Wayland compositor library). Plugins cannot compile with these internal headers.

**Solution**: The Plugin API provides:
1. **Opaque pointers** to compositor objects (`Tag*`, `Client*`, `Screen*`)
2. **Helper functions** to query these objects without needing full class definitions
3. **No wlroots dependencies** - plugins compile independently

## Helper Functions

All helpers are in the `Leviathan::UI::Plugin` namespace.

### Tag (Workspace) Helpers

```cpp
std::string GetTagName(Core::Tag* tag);
bool IsTagVisible(Core::Tag* tag);
int GetTagClientCount(Core::Tag* tag);
std::vector<Core::Client*> GetTagClients(Core::Tag* tag);
```

### Client (Window) Helpers

```cpp
std::string GetClientTitle(Core::Client* client);
std::string GetClientAppId(Core::Client* client);
bool IsClientFloating(Core::Client* client);
bool IsClientFullscreen(Core::Client* client);
```

### Screen (Monitor) Helpers

```cpp
std::string GetScreenName(Core::Screen* screen);
int GetScreenWidth(Core::Screen* screen);
int GetScreenHeight(Core::Screen* screen);
Core::Tag* GetScreenCurrentTag(Core::Screen* screen);
```

## Example Usage

```cpp
#include "ui/PluginAPI.hpp"

class MyWidget : public Leviathan::UI::WidgetPlugin {
public:
    void Update() override {
        auto* state = Leviathan::UI::GetCompositorState();
        if (!state) return;
        
        // Get active tag
        auto* tag = state->GetActiveTag();
        if (tag) {
            std::string name = Leviathan::UI::Plugin::GetTagName(tag);
            int clients = Leviathan::UI::Plugin::GetTagClientCount(tag);
            
            LOG_INFO("Active tag: {} with {} clients", name, clients);
        }
        
        // Enumerate all clients
        for (auto* client : state->GetAllClients()) {
            std::string title = Leviathan::UI::Plugin::GetClientTitle(client);
            std::string app = Leviathan::UI::Plugin::GetClientAppId(client);
            
            LOG_INFO("  {} - {}", app, title);
        }
    }
};
```

## Design Pattern

Plugins work with **opaque pointers**:
1. Get pointer from `CompositorState` (e.g., `GetTags()` returns `vector<Tag*>`)
2. Pass pointer to helper function (e.g., `GetTagName(tag)`)
3. Helper function (in compositor) calls actual method and returns result
4. Plugin never needs full `Tag` class definition

This keeps plugins **decoupled** from compositor internals while still providing full access to state.

## See Also

- `PLUGIN_DEV_GUIDE.md` - Complete development guide with build instructions
- `taglist-widget/` - Complete working example
- `clock-widget/` - Simpler example without compositor state
