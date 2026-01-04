---
weight: 5
title: "MenuBar & Launcher"
---

# MenuBar & Application Launcher

**Enhanced in v0.0.5**

The MenuBar provides a graphical application launcher with automatic desktop application discovery, custom commands, and bookmark integration.

## Features

✅ **Desktop Application Integration**
- Automatic scanning of `.desktop` files
- Support for 126+ system applications
- Category-based organization
- Application icons and descriptions

✅ **Custom Commands**
- Quick access to frequently used commands
- Shell command execution
- Custom labels and shortcuts

✅ **Bookmark Support**
- File manager bookmarks
- Quick directory access
- Custom bookmark definitions

✅ **Performance**
- Dirty region optimization
- Efficient rendering with Cairo
- Minimal memory footprint (~8MB for 126 items)

## Accessing the MenuBar

Press `Super + p` to open the application launcher.

The menubar appears at the top of the screen with all available applications organized by provider.

## Configuration

### Desktop Applications

The desktop application provider automatically scans:

- `/usr/share/applications/` - System-wide applications
- `~/.local/share/applications/` - User-installed applications
- `/var/lib/flatpak/exports/share/applications/` - Flatpak apps
- `~/.local/share/flatpak/exports/share/applications/` - User Flatpak apps

No configuration needed - it just works!

### Custom Commands

Add custom commands to `~/.config/leviathan/menubar.yaml`:

```yaml
custom_commands:
  - name: "Open Terminal"
    command: "alacritty"
    icon: "utilities-terminal"
    category: "System"
  
  - name: "System Monitor"
    command: "htop"
    icon: "system-monitor"
    category: "System"
  
  - name: "Take Screenshot"
    command: "grim -g \"$(slurp)\" screenshot.png"
    icon: "camera-photo"
    category: "Utilities"
```

### Bookmarks

Define custom bookmarks in `~/.config/leviathan/menubar.yaml`:

```yaml
bookmarks:
  - name: "Home Directory"
    path: "~/"
    icon: "user-home"
  
  - name: "Documents"
    path: "~/Documents"
    icon: "folder-documents"
  
  - name: "Projects"
    path: "~/Projects"
    icon: "folder-development"
```

Bookmarks open in your configured file manager (default: `thunar`).

## Architecture

### Provider System

The menubar uses a plugin-like provider architecture:

```cpp
class MenuBarProvider {
public:
    virtual std::vector<MenuItem> GetItems() = 0;
    virtual std::string GetName() const = 0;
};

// Built-in providers:
// 1. DesktopApplicationProvider - .desktop files
// 2. CustomCommandProvider - user-defined commands
// 3. BookmarkProvider - file/directory shortcuts
```

### Desktop Application Provider

Parses `.desktop` files following the freedesktop.org specification:

```cpp
MenuItem ParseDesktopFile(const std::string& path) {
    // Parse Name, Comment, Exec, Icon, Categories
    // Handle Exec field codes: %f, %F, %u, %U, %i, %c, %k
    // Filter by NoDisplay, Hidden, OnlyShowIn, NotShowIn
    // Return MenuItem with all metadata
}
```

**Supported Desktop Entry Keys:**
- `Name` - Display name
- `Comment` - Description tooltip
- `Exec` - Command to execute
- `Icon` - Icon name or path
- `Categories` - For organization
- `NoDisplay` - Hide from launcher
- `OnlyShowIn` / `NotShowIn` - DE filtering

### Rendering Pipeline

1. **Provider Query** - Get items from all providers
2. **Sorting** - Alphabetical by category then name
3. **Layout** - Calculate grid positions
4. **Cairo Rendering** - Draw to SHM buffer
5. **Scene Integration** - Attach to wlr_scene

**Dirty Region Optimization:**
```cpp
void MenuBar::CheckDirty() {
    if (needs_redraw_) {
        Render();
        UpdateBuffer();
        needs_redraw_ = false;
    }
}
```

Only re-renders when:
- Provider content changes
- Configuration reloaded
- Theme/styling updated

## Keybindings

Default keybindings in `~/.config/leviathan/leviathanrc`:

```bash
# Open application launcher
bind Super p open-launcher

# Close menubar (when open)
bind Escape close-menubar

# Navigate menu items
bind Down menu-next
bind Up menu-prev
bind Return menu-execute
```

## Styling

Customize menubar appearance in `~/.config/leviathan/theme.yaml`:

```yaml
menubar:
  background: "#2e3440"
  foreground: "#d8dee9"
  border: "#4c566a"
  
  item:
    background: "transparent"
    background_hover: "#3b4252"
    foreground: "#eceff4"
    
  icon:
    size: 48
    padding: 8
  
  font:
    family: "Sans"
    size: 12
```

## Performance

### Benchmarks

| Operation | Time |
|-----------|------|
| Initial scan (126 apps) | ~150ms |
| Menu open | <10ms |
| Item render | <5ms |
| Click response | <1ms |

### Memory Usage

- Base menubar: ~3MB
- 126 desktop items: ~5MB
- Total: ~8MB

### Optimization Tips

**For Fast Startup:**
- Limit desktop file locations
- Use category filters
- Cache parsed .desktop files

**For Low Memory:**
- Disable icon rendering
- Reduce item preview size
- Limit number of providers

## Advanced Usage

### Category Filtering

Filter which categories appear in the launcher:

```yaml
menubar:
  allowed_categories:
    - "Development"
    - "Graphics"
    - "Internet"
    - "Office"
    - "System"
  
  blocked_categories:
    - "Settings"
    - "Screensaver"
```

### Icon Theme Support

Configure icon theme search paths:

```yaml
menubar:
  icon_themes:
    - "Papirus-Dark"
    - "Adwaita"
    - "hicolor"
  
  icon_paths:
    - "~/.local/share/icons"
    - "/usr/share/icons"
    - "/usr/share/pixmaps"
```

### Launch with Arguments

Use Exec field codes in custom commands:

```yaml
custom_commands:
  - name: "Open File"
    command: "alacritty -e vim %f"
    icon: "text-editor"
    # %f = file path (will prompt user)
```

**Supported Exec Codes:**
- `%f` - Single file
- `%F` - Multiple files
- `%u` - Single URL
- `%U` - Multiple URLs
- `%i` - Icon with `--icon` prefix
- `%c` - Translated name
- `%k` - Desktop file path

## D-Bus Integration

The menubar supports D-Bus activation for launching apps:

```cpp
// Prefer D-Bus activation when available
if (desktop_entry.has_dbus_activatable) {
    ActivateViaDBus(desktop_entry.dbus_name);
} else {
    ExecuteCommand(desktop_entry.exec);
}
```

**Benefits:**
- Single instance management
- Proper startup notification
- Better resource management

## Troubleshooting

### Applications Not Appearing

**Check desktop file locations:**
```bash
ls /usr/share/applications/
ls ~/.local/share/applications/
```

**Verify desktop file format:**
```bash
desktop-file-validate /usr/share/applications/yourapp.desktop
```

**Enable debug logging:**
```yaml
logging:
  level: debug
  modules:
    - menubar
```

### Icons Not Loading

**Check icon theme:**
```bash
echo $GTK_ICON_THEME
```

**Verify icon exists:**
```bash
find /usr/share/icons -name "youricon.png"
```

**Use absolute paths:**
```yaml
custom_commands:
  - icon: "/usr/share/pixmaps/myicon.png"
```

### Slow Menu Opening

**Profile provider load time:**
- Check number of .desktop files
- Reduce scan locations
- Enable caching

**Monitor logs:**
```bash
./build/leviathan 2>&1 | grep "Loaded.*items"
```

## API Reference

### MenuItem Structure

```cpp
struct MenuItem {
    std::string name;           // Display name
    std::string comment;        // Description
    std::string command;        // Exec command
    std::string icon;           // Icon name/path
    std::string category;       // Category string
    bool terminal;              // Run in terminal?
    bool no_display;           // Hidden item?
};
```

### MenuBarProvider Interface

```cpp
class MenuBarProvider {
public:
    virtual ~MenuBarProvider() = default;
    
    // Get all menu items
    virtual std::vector<MenuItem> GetItems() = 0;
    
    // Provider display name
    virtual std::string GetName() const = 0;
    
    // Refresh items (called on config reload)
    virtual void Refresh() {}
};
```

## See Also

- [Configuration Guide](/docs/getting-started/configuration) - General configuration
- [Keybindings](/docs/getting-started/keybindings) - Keyboard shortcuts
- [Widget System](/docs/development/widget-system) - UI widget development
- [Status Bar](/docs/features/status-bar) - Status bar customization
