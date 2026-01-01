# MenuBar Usage Examples

The abstract MenuBar system allows you to create a launcher that can display not just applications, but any custom items you want.

## Basic Setup

```cpp
#include "ui/MenuBar.hpp"
#include "ui/MenuItemProviders.hpp"

// Create menubar configuration
UI::MenuBarConfig config;
config.height = 40;
config.item_height = 35;
config.max_visible_items = 8;
config.fuzzy_matching = true;

// Create menubar instance
auto menubar = std::make_unique<UI::MenuBar>(
    config,
    layer_manager,
    event_loop,
    output_width,
    output_height
);
```

## Adding Desktop Applications

```cpp
// Add desktop applications (.desktop files)
auto app_provider = std::make_shared<UI::DesktopApplicationProvider>();
menubar->AddProvider(app_provider);
```

## Adding Custom Commands

```cpp
// Create custom command provider
auto cmd_provider = std::make_shared<UI::CustomCommandProvider>();

// Add shell commands
cmd_provider->AddCommand(
    "Terminal",
    "alacritty",
    "Open a terminal emulator",
    100  // High priority
);

cmd_provider->AddCommand(
    "Screenshot",
    "grim -g \"$(slurp)\" ~/screenshots/$(date +%Y-%m-%d_%H-%M-%S).png",
    "Take a screenshot",
    90
);

cmd_provider->AddCommand(
    "Lock Screen",
    "swaylock -f",
    "Lock the screen",
    80
);

// Add function-based commands
cmd_provider->AddCommand(
    "Reload Config",
    []() {
        LOG_INFO("Reloading configuration...");
        // Your reload logic here
    },
    "Reload LeviathanDM configuration",
    70
);

menubar->AddProvider(cmd_provider);
```

## Adding Bookmarks

```cpp
// Create bookmark provider
auto bookmark_provider = std::make_shared<UI::BookmarkProvider>();

// Add directory bookmarks
bookmark_provider->AddBookmark(
    "Home",
    "/home/user",
    UI::BookmarkMenuItem::BookmarkType::Directory,
    "Home directory"
);

bookmark_provider->AddBookmark(
    "Projects",
    "/home/user/Projects",
    UI::BookmarkMenuItem::BookmarkType::Directory,
    "Project files"
);

// Add URL bookmarks
bookmark_provider->AddBookmark(
    "GitHub",
    "https://github.com",
    UI::BookmarkMenuItem::BookmarkType::URL,
    "Open GitHub"
);

// Add file bookmarks
bookmark_provider->AddBookmark(
    "Config",
    "/home/user/.config/leviathan/leviathan.yaml",
    UI::BookmarkMenuItem::BookmarkType::File,
    "Edit configuration"
);

menubar->AddProvider(bookmark_provider);
```

## Showing/Hiding the MenuBar

```cpp
// Show menubar (typically bound to a keybinding like Mod+P)
menubar->Show();

// Hide menubar
menubar->Hide();

// Toggle visibility
menubar->Toggle();
```

## Creating Custom Providers

You can create your own providers for any type of item:

```cpp
class RecentFilesProvider : public UI::IMenuItemProvider {
public:
    std::string GetName() const override {
        return "Recent Files";
    }
    
    std::vector<std::shared_ptr<UI::MenuItem>> LoadItems() override {
        std::vector<std::shared_ptr<UI::MenuItem>> items;
        
        // Load recent files from history
        auto recent_files = GetRecentFiles();
        
        for (const auto& file : recent_files) {
            items.push_back(std::make_shared<FileMenuItem>(file));
        }
        
        return items;
    }
    
private:
    std::vector<std::string> GetRecentFiles() {
        // Your logic to get recent files
        return {"/path/to/file1.txt", "/path/to/file2.cpp"};
    }
};

// Use it
auto recent_provider = std::make_shared<RecentFilesProvider>();
menubar->AddProvider(recent_provider);
```

## Custom Menu Items

```cpp
class FileMenuItem : public UI::MenuItem {
public:
    FileMenuItem(const std::string& filepath) : filepath_(filepath) {}
    
    std::string GetDisplayName() const override {
        return std::filesystem::path(filepath_).filename().string();
    }
    
    std::vector<std::string> GetSearchKeywords() const override {
        return {filepath_, GetDisplayName()};
    }
    
    std::string GetDescription() const override {
        return filepath_;
    }
    
    void Execute() override {
        // Open file with default application
        std::string cmd = "xdg-open \"" + filepath_ + "\"";
        system(cmd.c_str());
    }
    
private:
    std::string filepath_;
};
```

## Integration with Key Bindings

```cpp
// In your keybinding configuration
key_bindings->AddBinding(
    KEY_P,  // P key
    WLR_MODIFIER_LOGO,  // Super/Mod key
    [menubar = menubar.get()]() {
        menubar->Toggle();
    },
    "Toggle menubar"
);

// Handle keyboard input when menubar is visible
void HandleKeyboardInput(uint32_t key, const std::string& text) {
    if (menubar->IsVisible()) {
        switch (key) {
            case KEY_ESC:
                menubar->HandleEscape();
                break;
            case KEY_ENTER:
                menubar->HandleEnter();
                break;
            case KEY_BACKSPACE:
                menubar->HandleBackspace();
                break;
            case KEY_UP:
                menubar->HandleArrowUp();
                break;
            case KEY_DOWN:
                menubar->HandleArrowDown();
                break;
            default:
                if (!text.empty()) {
                    menubar->HandleTextInput(text);
                }
                break;
        }
    }
}
```

## Configuration Options

```cpp
UI::MenuBarConfig config;

// Visual
config.height = 40;                    // Input field height
config.item_height = 35;               // Menu item height
config.max_visible_items = 8;          // Max items visible before scrolling
config.padding = 10;                   // Padding around elements

// Colors
config.background_color = {0.1, 0.1, 0.1, 0.95};  // Dark semi-transparent
config.selected_color = {0.2, 0.4, 0.6, 1.0};     // Blue highlight
config.text_color = {1.0, 1.0, 1.0, 1.0};         // White text
config.description_color = {0.7, 0.7, 0.7, 1.0};  // Gray description

// Fonts
config.font_family = "Sans";
config.font_size = 12;
config.description_font_size = 9;

// Behavior
config.fuzzy_matching = true;          // Enable fuzzy search
config.case_sensitive = false;         // Case-insensitive search
config.min_chars_for_search = 0;       // Start filtering immediately
```

## Example Use Cases

### 1. Session Management
```cpp
cmd_provider->AddCommand("Logout", "loginctl terminate-user $USER", "End session", 100);
cmd_provider->AddCommand("Reboot", "systemctl reboot", "Reboot system", 90);
cmd_provider->AddCommand("Shutdown", "systemctl poweroff", "Shutdown system", 90);
```

### 2. Window Management
```cpp
cmd_provider->AddCommand("Kill Window", [compositor]() {
    compositor->EnterKillMode();
}, "Click to close a window", 50);
```

### 3. Quick Settings
```cpp
cmd_provider->AddCommand("Toggle WiFi", "nmcli radio wifi toggle", "Toggle WiFi on/off");
cmd_provider->AddCommand("Bluetooth Settings", "blueman-manager", "Open Bluetooth settings");
```

### 4. Development Shortcuts
```cpp
bookmark_provider->AddBookmark("LeviathanDM", "/Projects/LeviathanDM", 
    BookmarkType::Directory, "Main project");
cmd_provider->AddCommand("Build Project", "cd /Projects/LeviathanDM && make", 
    "Build LeviathanDM");
```

## Priority System

Items with higher priority appear first in the menu:

- **100+**: Critical items (logout, shutdown)
- **75-99**: Frequently used items (terminal, browser)
- **50-74**: Regular items
- **25-49**: Less common items
- **0-24**: Rarely used items
- **Negative**: Hidden by default

Desktop applications typically have priority 0 (default).
