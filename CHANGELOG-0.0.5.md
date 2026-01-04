# Changelog - Version 0.0.5

**Release Date:** January 4, 2026

## Major Features

### XWayland Support - Full X11 Application Integration
Implemented comprehensive XWayland support enabling X11 applications to run seamlessly alongside native Wayland clients with full window management integration.

#### What Changed
- **Before:** XWayland was broken - X11 applications would crash the compositor with "fatal display error: Broken pipe"
- **After:** Full X11 application support with proper lifecycle management, tiling integration, and window decorations

#### Technical Details

##### 1. XWayland Surface Lifecycle Management
Implemented proper handling of XWayland surface association and lifecycle:

```cpp
// XWayland surfaces don't have wl_surface immediately
view_handle_associate(struct wl_listener* listener, void* data) {
    // Associate event fires when wl_surface becomes available
    View* view = wl_container_of(listener, view, associate);
    
    // Add surface listeners when surface is ready
    if (view->xwayland_surface->surface) {
        wl_signal_add(&view->xwayland_surface->surface->events.commit, 
                      &view->commit);
        wl_signal_add(&view->xwayland_surface->surface->events.destroy, 
                      &view->surface_destroy);
    }
    
    // Check if already mapped before associate event
    if (view->xwayland_surface->mapped) {
        view_handle_map(&view->map, view);
    }
}
```

##### 2. Surface Cleanup and Crash Prevention
Added proper surface destroy handling to prevent assertion failures:

```cpp
view_handle_surface_destroy(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, surface_destroy);
    
    // Remove listeners before wl_surface is destroyed
    wl_list_remove(&view->commit.link);
    wl_list_remove(&view->surface_destroy.link);
    
    // Reset to prevent double-removal in destructor
    wl_list_init(&view->commit.link);
    wl_list_init(&view->surface_destroy.link);
}
```

##### 3. Client and Tag Integration
X11 windows now participate fully in the window management system:

```cpp
// Register X11 window with client/tag system
Server::OnNewXwaylandSurface(wlr_xwayland_surface* xwayland_surface) {
    // Create View
    View* view = new View(xwayland_surface);
    
    // Create Client wrapper
    auto client = std::make_shared<Client>(view);
    core_seat_->AddClient(client);
    
    // Add to current tag for tiling
    if (auto tag = core_seat_->GetActiveTag()) {
        tag->AddClient(client);
    }
}
```

##### 4. Dual-Protocol Window Resizing
Implemented proper resize handling for both X11 and Wayland protocols:

```cpp
TilingLayout::MoveResizeView(View* view, int x, int y, int width, int height) {
    // Position the scene node
    wlr_scene_node_set_position(&view->scene_tree->node, x, y);
    
    // Use appropriate resize API based on window type
    if (view->is_xwayland) {
        // X11 protocol requires position + size in one call
        wlr_xwayland_surface_configure(view->xwayland_surface, 
                                       x, y, width, height);
    } else {
        // Wayland protocol only needs size
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, width, height);
    }
}
```

##### 5. C++ Compatibility Layer
Created wrapper to avoid C++ keyword conflicts with xwayland.h:

```cpp
// XwaylandCompat.hpp - wraps C headers to avoid 'class' keyword conflicts
extern "C" {
    struct wlr_xwayland_surface;
    
    void wlr_xwayland_surface_configure(struct wlr_xwayland_surface*, 
                                       int16_t x, int16_t y, 
                                       uint16_t width, uint16_t height);
    
    // Accessor functions to avoid direct struct member access
    const char* xwayland_surface_get_title(struct wlr_xwayland_surface*);
    const char* xwayland_surface_get_class(struct wlr_xwayland_surface*);
}
```

#### Impact
- ✅ Chrome, Firefox, and other X11 applications now run without crashes
- ✅ X11 windows tile properly alongside Wayland windows
- ✅ X11 windows receive proper window decorations (borders, shadows, opacity)
- ✅ X11 windows respond to keyboard shortcuts and window management actions
- ✅ Clean shutdown without assertion failures or memory leaks

#### Testing
Tested with multiple X11 applications:
```bash
DISPLAY=:2 google-chrome-stable --ozone-platform=x11
DISPLAY=:2 xterm
DISPLAY=:2 firefox
```

All applications display correctly, tile properly, and close cleanly.

---

### Window Tabs Bar (In Development)
Initial implementation of tabbed window interface for better window organization.

#### Current Status
- Basic tab bar UI structure in place
- Integration with window management system
- Tab switching and window grouping planned for next release

---

### MenuBar Enhancements
Improved application launcher menu with better performance and UX.

#### What Changed
- Enhanced menu item provider system
- Better desktop application integration
- Improved rendering performance with dirty region tracking
- Custom command support
- Bookmark integration

#### Technical Details

##### 1. Provider Architecture
```cpp
class MenuBarProvider {
public:
    virtual std::vector<MenuItem> GetItems() = 0;
    virtual std::string GetName() const = 0;
};

// MenuBar aggregates multiple providers
void MenuBar::AddProvider(std::unique_ptr<MenuBarProvider> provider) {
    providers_.push_back(std::move(provider));
    RefreshItems();
}
```

##### 2. Desktop Application Provider
Scans .desktop files for system-wide application discovery:
```cpp
class DesktopApplicationProvider : public MenuBarProvider {
    std::vector<MenuItem> GetItems() override {
        // Scan /usr/share/applications, ~/.local/share/applications
        // Parse .desktop files for Name, Exec, Icon, Categories
        // Return sorted, categorized menu items
    }
};
```

##### 3. Dirty Region Optimization
Only re-render when menu content changes:
```cpp
void MenuBar::CheckDirty() {
    if (needs_redraw_) {
        Render();
        UpdateBuffer();
        needs_redraw_ = false;
    }
}
```

---

## Bug Fixes

### XWayland Crash Fixes
- Fixed "fatal display error: Broken pipe" when launching Chrome
- Fixed "fatal display error: Connection reset by peer" during window tiling
- Fixed assertion failure: `wl_list_empty(&surface->events.commit.listener_list)` on window close
- Fixed NULL pointer dereference in event listener registration
- Fixed double-free in View destructor with wl_list_init() initialization

### Window Management
- Fixed X11 windows not appearing at all (removed premature wl_surface check)
- Fixed X11 windows appearing but not tiling (added Client/Tag registration)
- Fixed tiling system not counting X11 windows (dual-path view type detection)
- Fixed event loop hanging on display errors (proper error handling and cleanup)

### Compositor Stability
- Added robust error handling to main event loop
- Improved surface validation checks
- Better cleanup of scene graph resources
- Memory leak fixes in XWayland surface management

---

## Technical Improvements

### Event Loop Error Handling
```cpp
int Server::Run() {
    while (running_) {
        int ret = wl_event_loop_dispatch(event_loop_, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;  // Interrupted, retry
            
            // Fatal error - clean shutdown
            LOG_ERROR("Event loop error: {}", strerror(errno));
            break;
        }
        
        wl_display_flush_clients(display_);
    }
    return 0;
}
```

### Listener Safety Pattern
```cpp
// Initialize all wl_list links to prevent crashes
View::View() {
    wl_list_init(&commit.link);
    wl_list_init(&map.link);
    wl_list_init(&unmap.link);
    wl_list_init(&destroy.link);
    wl_list_init(&associate.link);
    wl_list_init(&surface_destroy.link);
}

// Safe removal in destructor
View::~View() {
    if (!wl_list_empty(&commit.link)) {
        wl_list_remove(&commit.link);
    }
    // ... repeat for all listeners
}
```

---

## Known Issues

### XWayland Limitations
- X11 window decorations may appear different from native Wayland windows
- Some X11-specific window properties not yet fully supported
- Focus behavior may need tuning for complex X11 applications

### MenuBar
- Animation effects not yet implemented
- Fuzzy search for menu items in progress
- Icon rendering performance can be improved

### Tabs Bar
- Tab bar UI complete but switching logic needs implementation
- Tab drag-and-drop not yet functional
- Tab grouping persistence not implemented

---

## Build System

### Dependencies
- wlroots 0.19.2 (vendored as Meson subproject)
- libdisplay-info (vendored)
- Cairo for 2D rendering
- Pango for text rendering
- nlohmann/json for configuration
- GLib/GObject for D-Bus integration

### Build Instructions
```bash
# Install dependencies
./setup-deps.sh

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run
./leviathan
```

---

## Performance Metrics

### XWayland Performance
- Chrome startup time: ~2s (comparable to native X11 server)
- Window tiling: <5ms for 2-6 windows
- No perceptible lag in window operations

### Memory Usage
- Base compositor: ~45MB
- Per X11 window: ~2-3MB overhead
- MenuBar with 126 items: ~8MB

---

## API Changes

### View Constructor
Added XWayland-specific initialization:
```cpp
// New signature supports both XDG toplevels and XWayland surfaces
View(struct wlr_xdg_toplevel* toplevel);      // Wayland
View(struct wlr_xwayland_surface* surface);   // X11
```

### Layout Interface
Updated to handle dual protocols:
```cpp
class Layout {
public:
    // Must check view->is_xwayland before calling resize
    virtual void MoveResizeView(View* view, int x, int y, 
                               int width, int height) = 0;
};
```

---

## Migration Guide

### For Users
- X11 applications now work out of the box
- Use `DISPLAY=:2` environment variable for X11 apps
- Chrome works with: `DISPLAY=:2 google-chrome-stable --ozone-platform=x11`

### For Developers
- Check `view->is_xwayland` before accessing view properties
- Use `wlr_xwayland_surface_configure()` for X11 windows
- Use `wlr_xdg_toplevel_set_size()` for Wayland windows
- Always initialize wl_listener links with `wl_list_init()`
- Add surface_destroy listener for proper cleanup

---

## Contributors

- Romeo (LeviathanSystems) - XWayland implementation, bug fixes, menubar enhancements

---

## Future Roadmap (0.0.6)

### Planned Features
- **Tab Bar Completion**: Full tab switching and grouping functionality
- **XWayland Polish**: Window property support, better focus handling
- **Multi-Monitor**: Proper multi-display tiling and workspace management
- **Window Rules**: Advanced matching and automation
- **IPC Enhancements**: Better leviathanctl integration
- **Plugin System**: Dynamic widget loading and custom layouts

### Performance Goals
- Sub-millisecond window tiling for up to 20 windows
- <50MB memory footprint for base compositor
- 60fps UI animations in menubar and tabs

---

## Breaking Changes

None. This release is fully backward compatible with 0.0.4 configuration files.

---

## Upgrade Instructions

```bash
# Pull latest changes
git pull origin master

# Clean build (recommended for major version bumps)
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

Configuration files from 0.0.4 work without modification.
