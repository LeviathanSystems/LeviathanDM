---
weight: 1
title: "XWayland Support"
---

# XWayland Support

**Introduced in v0.0.5**

LeviathanDM now has full XWayland support, enabling X11 applications to run seamlessly alongside native Wayland clients with complete window management integration.

## What Works

✅ **X11 Applications**
- Chrome/Chromium browsers
- Firefox
- Legacy X11 applications (xterm, xcalc, etc.)
- Electron apps running in X11 mode
- Most X11-based GUI applications

✅ **Window Management**
- Full tiling support for X11 windows
- Window decorations (borders, shadows, opacity)
- Focus management and keyboard shortcuts
- Window close, minimize, maximize operations
- Mixed Wayland/X11 window tiling

✅ **Stability**
- Clean shutdown without crashes
- Proper surface lifecycle management
- No "Broken pipe" or "Connection reset" errors
- Memory leak-free operation

## Running X11 Applications

X11 applications automatically connect to the XWayland server running on display `:2`:

```bash
# Set DISPLAY environment variable
export DISPLAY=:2

# Launch X11 applications
google-chrome-stable --ozone-platform=x11
firefox
xterm
```

Or specify the display inline:

```bash
DISPLAY=:2 google-chrome-stable --ozone-platform=x11
DISPLAY=:2 firefox
```

## Chrome/Chromium Configuration

For the best Chrome experience with XWayland:

```bash
DISPLAY=:2 google-chrome-stable \
  --ozone-platform=x11 \
  --disable-features=WaylandWindowDecorations
```

**Flags explained:**
- `--ozone-platform=x11` - Force X11 backend instead of Wayland
- `--disable-features=WaylandWindowDecorations` - Use compositor decorations

### Creating a Desktop Entry

Create `~/.local/share/applications/chrome-xwayland.desktop`:

```ini
[Desktop Entry]
Name=Chrome (XWayland)
Comment=Google Chrome with X11 backend
Exec=env DISPLAY=:2 google-chrome-stable --ozone-platform=x11
Icon=google-chrome
Type=Application
Categories=Network;WebBrowser;
```

## Technical Architecture

### Dual-Protocol Window Management

LeviathanDM intelligently handles both Wayland (XDG toplevels) and X11 (XWayland surfaces) windows:

```cpp
if (view->is_xwayland) {
    // X11 protocol - requires position AND size
    wlr_xwayland_surface_configure(view->xwayland_surface, x, y, width, height);
} else {
    // Wayland protocol - only size (compositor controls position)
    wlr_xdg_toplevel_set_size(view->xdg_toplevel, width, height);
}
```

### Surface Lifecycle

X11 surfaces have a more complex lifecycle than Wayland surfaces:

1. **Creation** - `new_xwayland_surface` event fires
2. **Association** - `associate` event when `wl_surface` becomes available
3. **Mapping** - Window becomes visible (can happen before association!)
4. **Destruction** - Surface cleanup with proper listener removal

```cpp
// Handle late surface association
void view_handle_associate(struct wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, associate);
    
    // Now we can add surface-specific listeners
    if (view->xwayland_surface->surface) {
        wl_signal_add(&view->xwayland_surface->surface->events.commit, 
                      &view->commit);
        wl_signal_add(&view->xwayland_surface->surface->events.destroy, 
                      &view->surface_destroy);
    }
    
    // Check if window was already mapped before association
    if (view->xwayland_surface->mapped) {
        view_handle_map(&view->map, view);
    }
}
```

### C++ Compatibility Layer

The `xwayland.h` header uses C++ reserved keywords like `class`, requiring a compatibility wrapper:

```cpp
// XwaylandCompat.hpp
extern "C" {
    struct wlr_xwayland_surface;
    
    // Forward declarations for wlroots functions
    void wlr_xwayland_surface_configure(struct wlr_xwayland_surface*, 
                                       int16_t x, int16_t y, 
                                       uint16_t width, uint16_t height);
    
    // Accessor functions to avoid 'class' keyword
    const char* xwayland_surface_get_title(struct wlr_xwayland_surface*);
    const char* xwayland_surface_get_class(struct wlr_xwayland_surface*);
    pid_t xwayland_surface_get_pid(struct wlr_xwayland_surface*);
}
```

### Event Loop Error Handling

Robust error handling prevents compositor crashes from XWayland issues:

```cpp
int Server::Run() {
    while (running_) {
        int ret = wl_event_loop_dispatch(event_loop_, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;  // Interrupted, retry
            
            LOG_ERROR("Event loop error: {}", strerror(errno));
            break;  // Clean shutdown
        }
        
        wl_display_flush_clients(display_);
    }
    return 0;
}
```

## Known Limitations

### What Doesn't Work Yet

⚠️ **Window Properties**
- Some X11-specific window properties not fully implemented
- Window type hints (dialog, utility) may not be fully honored
- NET_WM hints partially supported

⚠️ **Focus Behavior**
- Focus stealing prevention for X11 apps needs tuning
- Input method (IME) support for X11 apps incomplete

⚠️ **Performance**
- X11 window rendering slightly slower than native Wayland
- Large X11 apps (IDEs) may have minor lag

### Compatibility Notes

**Works Well:**
- Web browsers (Chrome, Firefox)
- Terminal emulators (xterm, urxvt)
- Simple GUI applications
- Most Electron apps

**May Have Issues:**
- Complex IDEs (VSCode works but may have quirks)
- Games with custom window management
- Applications using exotic X11 extensions

## Debugging

### Enable XWayland Logging

Set environment variables for detailed logs:

```bash
export WAYLAND_DEBUG=1
export WLR_DEBUG=1
./build/leviathan 2>&1 | tee leviathan.log
```

### Check XWayland Connection

Verify XWayland server is running:

```bash
# Check display assignment
echo $DISPLAY
# Should show: :2

# List X clients
xlsclients -display :2
```

### Common Issues

**Problem:** X11 app doesn't appear

**Solution:**
1. Verify DISPLAY is set: `echo $DISPLAY`
2. Check compositor logs for errors
3. Try simpler X11 app first: `DISPLAY=:2 xterm`

**Problem:** App crashes with "Can't open display"

**Solution:**
- XWayland server may not be ready yet
- Wait 1-2 seconds after compositor start
- Check `leviathan.log` for "Xwayland server is ready"

**Problem:** Window decorations missing

**Solution:**
- Window rule may be disabling decorations
- Check `~/.config/leviathan/leviathanrc` for decoration rules
- Some X11 apps request no decorations

## Performance Comparison

| Operation | Native Wayland | XWayland |
|-----------|---------------|----------|
| Window creation | ~10ms | ~15ms |
| Window tiling | <5ms | <5ms |
| Focus switch | <1ms | <1ms |
| Memory per window | 2-3MB | 3-4MB |

XWayland overhead is minimal for most use cases.

## Future Improvements

Planned for v0.0.6 and beyond:

- [ ] Full NET_WM hint support
- [ ] Better input method (IME) integration
- [ ] X11 clipboard sync improvements
- [ ] Per-app XWayland/Wayland preference
- [ ] XWayland rootful mode option
- [ ] Improved window type handling

## References

- [wlroots XWayland Documentation](https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/xwayland)
- [Wayland Book - XWayland](https://wayland-book.com/)
- [X11 Protocol Specification](https://www.x.org/releases/current/doc/xproto/x11protocol.html)

## See Also

- [Window Management](/docs/features/layouts) - Tiling layout documentation
- [Configuration](/docs/getting-started/configuration) - Configure window decorations
- [Architecture](/docs/development/architecture) - Compositor architecture details
