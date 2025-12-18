# Help Window Implementation Summary

## Overview

Implemented an awesome-WM-style help window for LeviathanDM that displays all keybindings when `Super + F1` is pressed.

## Implementation Details

### 1. Layer-Shell Protocol Support
**Files Modified:**
- `include/wayland/LayerSurface.hpp` (created)
- `src/wayland/LayerSurface.cpp` (created)
- `include/wayland/Server.hpp`
- `src/wayland/Server.cpp`

**Features:**
- Full `wlr_layer_shell_v1` protocol support (version 4)
- Handles BACKGROUND, BOTTOM, TOP, and OVERLAY layers
- Routes to compositor's layer manager
- Supports bars, notifications, overlays, and backgrounds

**C++ Compatibility:**
- Layer-shell protocol uses `namespace` as a parameter name (C++ keyword)
- Solution: Localized `#define namespace namespace_` only in headers that include layer-shell
- Pattern used in LayerSurface.hpp and Server.hpp:
  ```cpp
  #define namespace namespace_
  extern "C" { #include <wlr/types/wlr_layer_shell_v1.h> }
  #undef namespace
  ```

### 2. Help Window Application
**Files Created:**
- `tools/help-window/main.c` - GTK4 application
- `tools/help-window/CMakeLists.txt` - Build configuration
- `tools/help-window/README.md` - Documentation

**Technology Stack:**
- **GTK4** - Modern UI toolkit
- **gtk-layer-shell** - Wayland layer-shell integration
- **C language** - Simple, lightweight implementation

**Features:**
- Displays all keybindings organized by category
- Dark theme with semi-transparent background
- Overlay layer (appears above all windows)
- Centered on screen
- Close with `Esc` or `F1`
- Scrollable content

**Categories:**
1. Window Management
2. Focus Navigation
3. Window Swapping
4. Layout Control
5. Layout Switching
6. Workspaces
7. Applications

### 3. Keybinding Integration
**Files Modified:**
- `src/KeyBindings.cpp` - Added `Super + F1` binding
- `CMakeLists.txt` - Added help window subdirectory

**Keybinding:**
```cpp
bindings_.push_back({mod, XKB_KEY_F1, [this]() {
    system("leviathan-help &");
}});
```

### 4. Build System
**Files Modified:**
- `CMakeLists.txt` - Added `add_subdirectory(tools/help-window)`

**Build Process:**
1. Main compositor builds as usual
2. Help window builds as separate executable
3. Both installed to `/usr/local/bin/` (or system prefix)

**Binary Locations:**
- Compositor: `build/leviathan`
- Help tool: `build/tools/help-window/leviathan-help`

### 5. Documentation
**Files Modified:**
- `README.md` - Added help window section, layer-shell feature, GTK dependencies

**New Sections:**
- Help Window feature description
- GTK4/gtk-layer-shell dependencies
- `Super + F1` keybinding documentation

## Dependencies Added

### Build-Time
- GTK4 development files (`libgtk-4-dev`)
- gtk-layer-shell development files (`libgtk-layer-shell-dev`)

### Runtime
- GTK4 libraries
- gtk-layer-shell libraries

## Usage

### Launch Help Window
Press `Super + F1` in the compositor

### Manual Launch
```bash
leviathan-help
```

### Building
```bash
./build.sh  # Builds both compositor and help window
```

## Technical Highlights

### Layer-Shell Configuration
```c
gtk_layer_init_for_window(GTK_WINDOW(window));
gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
```

### Keybinding Data Structure
```c
typedef struct {
    const char* keys;
    const char* description;
    const char* category;
} Keybinding;
```

### Styling
- Semi-transparent dark background: `rgba(30, 30, 30, 0.95)`
- White text for contrast
- Bold keybinding names
- Category headers in larger font
- Separators between sections

## Future Enhancements

### Possible Improvements
1. **Dynamic Keybinding Loading**: Read from YAML config or IPC instead of hardcoded
2. **Searchable Interface**: Add search box to filter keybindings
3. **Custom Themes**: Support light/dark theme switching
4. **Window Size Options**: Make window size configurable
5. **Multi-Monitor**: Show on focused monitor only
6. **Animations**: Fade in/out transitions

### IPC Integration
Could query compositor for current keybindings:
```cpp
// In compositor
void HandleGetKeybindings(Connection& conn) {
    json response;
    for (const auto& binding : keybindings) {
        response["keybindings"].push_back({
            {"keys", KeyToString(binding.modifiers, binding.keysym)},
            {"description", binding.description}
        });
    }
    conn.Send(response);
}
```

## Testing Checklist

- [x] Builds successfully
- [ ] Layer-shell protocol works
- [ ] Window appears on `Super + F1`
- [ ] Overlay layer positioning correct
- [ ] All keybindings displayed
- [ ] Scrolling works
- [ ] Close on Escape works
- [ ] Close on F1 works
- [ ] Styled correctly

## Files Changed Summary

### Created (7 files)
1. `include/wayland/LayerSurface.hpp` - Layer surface handler
2. `src/wayland/LayerSurface.cpp` - Layer surface implementation
3. `tools/help-window/main.c` - GTK help application
4. `tools/help-window/CMakeLists.txt` - Build config
5. `tools/help-window/README.md` - Tool documentation
6. This summary file

### Modified (5 files)
1. `include/wayland/Server.hpp` - Added layer-shell members
2. `src/wayland/Server.cpp` - Initialize layer-shell
3. `src/KeyBindings.cpp` - Added F1 keybinding
4. `CMakeLists.txt` - Added help window subdirectory
5. `README.md` - Updated documentation

## Conclusion

Successfully implemented a complete help window system including:
- ✅ Layer-shell protocol support for overlays
- ✅ GTK4 help window application
- ✅ Keybinding to launch (`Super + F1`)
- ✅ Build system integration
- ✅ Documentation

The implementation follows the awesome-WM pattern and provides users with an easy way to discover and remember keybindings.
