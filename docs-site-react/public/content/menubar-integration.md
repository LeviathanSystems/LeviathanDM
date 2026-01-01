# MenuBar Integration Example

This example shows how to integrate the MenuBar into your LeviathanDM setup.

## How It Works

The MenuBar is automatically initialized and registered for each output when it's created. You just need to:

1. Add item providers (applications, commands, bookmarks)
2. Use the keybinding (Mod+P by default) to show it

## Default Setup

The menubar is automatically:
- Initialized during compositor startup
- Registered for each monitor/output
- Bound to Mod+P key to toggle visibility
- Rendered in the top layer of each output

## Adding Custom Providers

Add this code during compositor initialization (in `main.cpp` or after config loading):

```cpp
#include "ui/MenuBarManager.hpp"
#include "ui/MenuItemProviders.hpp"

// Get menubar manager singleton
auto& menubar_mgr = UI::MenuBarManager::Instance();

// 1. Add desktop applications provider
auto app_provider = std::make_shared<UI::DesktopApplicationProvider>();
menubar_mgr.AddProvider(app_provider);

// 2. Add custom commands provider
auto cmd_provider = std::make_shared<UI::CustomCommandProvider>();

// Add frequently used commands
cmd_provider->AddCommand(
    "Terminal",
    "alacritty",
    "Open Alacritty terminal",
    100  // High priority - appears first
);

cmd_provider->AddCommand(
    "Browser",
    "firefox",
    "Open Firefox web browser",
    95
);

cmd_provider->AddCommand(
    "Screenshot",
    "grim -g \"$(slurp)\" ~/screenshots/$(date +%Y-%m-%d_%H-%M-%S).png",
    "Take a screenshot of selected area",
    90
);

cmd_provider->AddCommand(
    "Screenshot (Full)",
    "grim ~/screenshots/$(date +%Y-%m-%d_%H-%M-%S).png",
    "Take a full screenshot",
    89
);

// System commands
cmd_provider->AddCommand(
    "Lock Screen",
    "swaylock -f -c 000000",
    "Lock the screen",
    80
);

cmd_provider->AddCommand(
    "Logout",
    "loginctl terminate-user $USER",
    "End session and logout",
    75
);

// Add function-based command
cmd_provider->AddCommand(
    "Reload Config",
    []() {
        auto& config = Leviathan::Config();
        config.Reload();
        LOG_INFO("Configuration reloaded");
    },
    "Reload LeviathanDM configuration",
    70
);

menubar_mgr.AddProvider(cmd_provider);

// 3. Add bookmarks provider
auto bookmark_provider = std::make_shared<UI::BookmarkProvider>();

// Add directory bookmarks
bookmark_provider->AddBookmark(
    "Home",
    std::string(getenv("HOME")),
    UI::BookmarkMenuItem::BookmarkType::Directory,
    "Home directory"
);

bookmark_provider->AddBookmark(
    "Projects",
    std::string(getenv("HOME")) + "/Projects",
    UI::BookmarkMenuItem::BookmarkType::Directory,
    "Projects folder"
);

bookmark_provider->AddBookmark(
    "Downloads",
    std::string(getenv("HOME")) + "/Downloads",
    UI::BookmarkMenuItem::BookmarkType::Directory,
    "Downloads folder"
);

// Add URL bookmarks
bookmark_provider->AddBookmark(
    "GitHub",
    "https://github.com",
    UI::BookmarkMenuItem::BookmarkType::URL,
    "Open GitHub"
);

bookmark_provider->AddBookmark(
    "Documentation",
    "https://leviathandm.readthedocs.io",
    UI::BookmarkMenuItem::BookmarkType::URL,
    "LeviathanDM Documentation"
);

// Add file bookmarks
bookmark_provider->AddBookmark(
    "Config File",
    std::string(getenv("HOME")) + "/.config/leviathan/leviathan.yaml",
    UI::BookmarkMenuItem::BookmarkType::File,
    "Edit LeviathanDM configuration"
);

menubar_mgr.AddProvider(bookmark_provider);
```

## Usage

1. Press `Mod+P` (Super+P) to show the menubar on the focused screen
2. Type to search/filter items (supports fuzzy matching)
3. Use arrow keys (Up/Down) to navigate
4. Press Enter to execute the selected item
5. Press Escape to hide the menubar

## Customization

### Change Keybinding

Edit `src/KeyBindings.cpp`:

```cpp
// Change from XKB_KEY_p to another key
bindings_.push_back({mod, XKB_KEY_d, [this]() {  // Now Mod+D
    // ... menubar toggle code ...
}});
```

### Customize Appearance

```cpp
auto& menubar_mgr = UI::MenuBarManager::Instance();
UI::MenuBarConfig config;

// Visual properties
config.height = 45;                    // Input field height
config.item_height = 40;               // Menu item height
config.max_visible_items = 10;         // Show more items

// Colors (RGBA)
config.background_color = {0.05, 0.05, 0.05, 0.98};  // Darker, more opaque
config.selected_color = {0.3, 0.5, 0.7, 1.0};         // Brighter blue
config.text_color = {0.95, 0.95, 0.95, 1.0};          // Slightly off-white
config.description_color = {0.6, 0.6, 0.6, 1.0};      // Lighter gray

// Fonts
config.font_family = "JetBrains Mono";
config.font_size = 13;
config.description_font_size = 10;

// Behavior
config.fuzzy_matching = true;          // Enable fuzzy search
config.case_sensitive = false;         // Case-insensitive
config.min_chars_for_search = 1;       // Start filtering after 1 character

menubar_mgr.SetConfig(config);
```

## Creating Custom Providers

You can create custom providers for any type of content:

```cpp
// Example: Recent files provider
class RecentFilesProvider : public UI::IMenuItemProvider {
public:
    std::string GetName() const override {
        return "Recent Files";
    }
    
    std::vector<std::shared_ptr<UI::MenuItem>> LoadItems() override {
        std::vector<std::shared_ptr<UI::MenuItem>> items;
        
        // Load recent files from your history
        auto recent = LoadRecentFilesFromHistory();
        
        for (const auto& file : recent) {
            items.push_back(std::make_shared<FileMenuItem>(file));
        }
        
        return items;
    }
};

// Add to menubar
auto recent_provider = std::make_shared<RecentFilesProvider>();
menubar_mgr.AddProvider(recent_provider);
```

## Per-Monitor Configuration

The menubar automatically appears on the active/focused monitor. Each monitor has its own menubar instance, but they share the same item providers and configuration.

## Dynamic Updates

To refresh items (e.g., after adding new commands):

```cpp
auto& menubar_mgr = UI::MenuBarManager::Instance();
menubar_mgr.RefreshAllItems();  // Reloads all items from all providers
```

## Integration with Status Bar

The menubar appears in the top layer, above status bars and windows. It doesn't reserve any screen space - it only appears on-demand as an overlay.
