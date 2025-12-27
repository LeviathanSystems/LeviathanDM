# Version 0.0.2 Release Notes

## New Features

### Window Decorations System
A comprehensive window styling system has been added in version 0.0.2, providing advanced customization options for window appearance.

#### Features:
- **Customizable Borders**: Configure width, colors (focused/unfocused), and rounded corners
- **Window Opacity**: Separate opacity levels for focused and inactive windows (transparency support)
- **Drop Shadows**: Optional shadows with configurable size, color, opacity, and offset
- **Window Dimming**: Automatically dim inactive windows
- **Pattern Matching**: Apply styles based on app_id, window title, or floating state using wildcards
- **Named Decoration Groups**: Create reusable style presets for different window types

#### Configuration:
Two new configuration sections added to `leviathan.yaml`:
- `window-decorations`: Define named decoration styles
- `window-rules`: Apply decorations to windows using pattern matching

#### Example Use Cases:
- Transparent terminals (kitty, Alacritty, etc.)
- Borderless gaming windows
- Fancy floating windows with shadows and rounded corners
- Application-specific styling (media players, development tools)

### Version Support
Added `--version` and `--help` command-line flags:
```bash
leviathan --version  # Show version information
leviathan --help     # Show usage help
```

Version information is now displayed on startup in the logs.

## Removed Features

### Legacy Border Configuration
The following settings have been removed from the `general` section:
- `border_width`
- `border_color_focused`
- `border_color_unfocused`

These are now handled exclusively by the new window decorations system, providing more flexibility and per-window customization.

## Migration Guide

### From 0.0.1 to 0.0.2

If you had border settings in your `general` section:

**Old configuration (0.0.1):**
```yaml
general:
  border_width: 2
  border_color_focused: "#5E81AC"
  border_color_unfocused: "#3B4252"
```

**New configuration (0.0.2):**
```yaml
window-decorations:
  - name: default
    border_width: 2
    border_color_focused: "#5E81AC"
    border_color_unfocused: "#3B4252"
    opacity: 1.0
    opacity_inactive: 1.0
    border_radius: 0
    enable_shadows: false

window-rules:
  - name: default-all
    app_id: "*"
    decoration_group: default
```

## Documentation

Full documentation for window decorations is available at:
- `/docs/features/window-decorations` - Comprehensive guide with examples
- Includes configuration reference, pattern matching guide, and troubleshooting

## Technical Details

### Files Modified:
- `include/config/ConfigParser.hpp` - Added WindowDecorationConfig and WindowRuleConfig
- `src/config/ConfigParser.cpp` - Added parsing for decorations and rules
- `include/Types.hpp` - Added decoration properties to View struct
- `src/wayland/View.cpp` - Implemented automatic decoration application
- `CMakeLists.txt` - Added version support with version.h generation
- `src/main.cpp` - Added --version and --help flags
- `VERSION` - Updated to 0.0.2

### Version System:
- Version is now read from `VERSION` file
- CMake generates `version.h` with version macros
- Version displayed on startup and via `--version` flag

## Notes

- Window decorations are applied automatically when windows are mapped
- Pattern matching supports wildcards: `*`, `prefix*`, `*substring*`
- Match priority: exact > substring > prefix > wildcard
- Documentation is version-aware and only shows features available in selected version
