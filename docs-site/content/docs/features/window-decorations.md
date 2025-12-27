---
weight: 5
title: "Window Decorations"
minVersion: "0.0.2"
---

# Window Decorations

{{< hint info >}}
**Since version 0.0.2**  
Window decorations provide advanced styling capabilities for windows with customizable borders, opacity, shadows, and more.
{{< /hint >}}

## Overview

LeviathanDM's window decoration system allows you to create reusable styling configurations and apply them to windows using pattern-matched rules. This provides a flexible way to customize the appearance of different applications.

## Features

- **Customizable Borders**: Width, colors (focused/unfocused), and rounded corners
- **Window Opacity**: Separate opacity levels for focused and inactive windows
- **Drop Shadows**: Optional shadows with configurable size, color, and opacity
- **Window Dimming**: Automatically dim inactive windows
- **Pattern Matching**: Apply styles based on app_id, window title, or floating state
- **Named Decoration Groups**: Create reusable style presets

## Configuration

### Window Decorations

Define reusable decoration styles in your `leviathan.yaml`:

```yaml
window-decorations:
  # Default decoration for most windows
  - name: default
    border_width: 2
    border_color_focused: "#5E81AC"      # Nord blue
    border_color_unfocused: "#3B4252"    # Nord dark gray
    opacity: 1.0                          # Fully opaque
    opacity_inactive: 1.0
    border_radius: 0                      # No rounded corners
    enable_shadows: false
    dim_inactive: false
  
  # Transparent terminal style
  - name: transparent
    border_width: 1
    border_color_focused: "#8FBCBB"      # Nord cyan
    border_color_unfocused: "#434C5E"
    opacity: 0.90                         # 90% opacity when focused
    opacity_inactive: 0.75                # 75% when unfocused
    border_radius: 8
    enable_shadows: true
    shadow_size: 10
    shadow_color: "#000000"
    shadow_opacity: 0.4
    dim_inactive: true
    dim_amount: 0.3
  
  # Floating windows with fancy styling
  - name: floating
    border_width: 3
    border_color_focused: "#88C0D0"      # Nord bright blue
    border_color_unfocused: "#4C566A"
    opacity: 1.0
    opacity_inactive: 0.95
    border_radius: 12                     # Rounded corners
    enable_shadows: true
    shadow_size: 15
    shadow_color: "#000000"
    shadow_opacity: 0.6
    shadow_offset_x: 0
    shadow_offset_y: 4
    dim_inactive: false
```

### Window Rules

Apply decorations to specific windows using rules:

```yaml
window-rules:
  # Terminal applications - transparent
  - name: kitty-terminal
    app_id: "kitty"
    decoration_group: transparent
  
  - name: all-terminals
    app_id: "*term*"                     # Matches any terminal
    decoration_group: transparent
  
  # Floating windows - fancy style
  - name: floating-windows
    match_mode: floating
    decoration_group: floating
  
  # Specific application by title
  - name: spotify
    title: "Spotify"
    decoration_group: transparent
    force_floating: true
  
  # Default catch-all - must be last
  - name: default-all
    app_id: "*"
    decoration_group: default
```

## Decoration Properties

### Border Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `border_width` | integer | `2` | Width of window borders in pixels |
| `border_color_focused` | string | `"#5E81AC"` | Border color for focused window (hex) |
| `border_color_unfocused` | string | `"#3B4252"` | Border color for unfocused windows (hex) |
| `border_radius` | integer | `0` | Corner radius in pixels (rounded corners) |

### Opacity Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `opacity` | float | `1.0` | Window opacity when focused (0.0-1.0) |
| `opacity_inactive` | float | `1.0` | Window opacity when unfocused (0.0-1.0) |
| `dim_inactive` | boolean | `false` | Dim unfocused windows |
| `dim_amount` | float | `0.3` | Dimming amount (0.0-1.0) |

### Shadow Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `enable_shadows` | boolean | `false` | Enable drop shadows |
| `shadow_size` | integer | `10` | Shadow blur radius in pixels |
| `shadow_color` | string | `"#000000"` | Shadow color (hex) |
| `shadow_opacity` | float | `0.5` | Shadow opacity (0.0-1.0) |
| `shadow_offset_x` | integer | `0` | Horizontal shadow offset |
| `shadow_offset_y` | integer | `0` | Vertical shadow offset |

## Rule Properties

### Matching Properties

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | **Required.** Unique identifier for the rule |
| `app_id` | string | Match by application ID (supports wildcards) |
| `title` | string | Match by window title (supports wildcards) |
| `match_mode` | string | Match by window state: `floating` or `tiled` |

### Action Properties

| Property | Type | Description |
|----------|------|-------------|
| `decoration_group` | string | Name of decoration style to apply |
| `force_floating` | boolean | Force window to float |
| `override_opacity` | float | Override decoration's opacity value |

## Pattern Matching

Window rules support wildcard pattern matching:

- **Exact match**: `"kitty"` - matches exactly "kitty"
- **Prefix match**: `"kitty*"` - matches "kitty", "kitty-terminal", etc.
- **Substring match**: `"*term*"` - matches "terminal", "xterm", "kitty-term", etc.
- **Match all**: `"*"` - matches any window

### Match Priority

When multiple rules match a window, the best match is selected:

1. **Exact match** - highest priority
2. **Substring match** - medium priority
3. **Prefix match** - lower priority
4. **Wildcard match** (`*`) - lowest priority

### Match Mode

Use `match_mode` to match windows based on their state:

```yaml
- name: floating-windows
  match_mode: floating
  decoration_group: floating

- name: tiled-windows
  match_mode: tiled
  decoration_group: default
```

## Examples

### Transparent Terminals

```yaml
window-decorations:
  - name: transparent-terminal
    border_width: 1
    border_color_focused: "#88C0D0"
    border_color_unfocused: "#434C5E"
    opacity: 0.90
    opacity_inactive: 0.75
    border_radius: 8
    enable_shadows: true
    shadow_size: 10
    shadow_color: "#000000"
    shadow_opacity: 0.4

window-rules:
  - name: kitty
    app_id: "kitty"
    decoration_group: transparent-terminal
  
  - name: alacritty
    app_id: "Alacritty"
    decoration_group: transparent-terminal
```

### Gaming Mode (Borderless)

```yaml
window-decorations:
  - name: gaming
    border_width: 0
    opacity: 1.0
    opacity_inactive: 1.0
    border_radius: 0
    enable_shadows: false
    dim_inactive: false

window-rules:
  - name: steam-games
    app_id: "*steam*"
    decoration_group: gaming
  
  - name: minecraft
    title: "Minecraft*"
    decoration_group: gaming
```

### Fancy Floating Windows

```yaml
window-decorations:
  - name: fancy-float
    border_width: 3
    border_color_focused: "#88C0D0"
    border_color_unfocused: "#4C566A"
    opacity: 1.0
    opacity_inactive: 0.95
    border_radius: 12
    enable_shadows: true
    shadow_size: 15
    shadow_color: "#000000"
    shadow_opacity: 0.6
    shadow_offset_y: 4

window-rules:
  - name: floating-windows
    match_mode: floating
    decoration_group: fancy-float
```

### Application-Specific Styling

```yaml
window-decorations:
  - name: media-player
    border_width: 2
    border_color_focused: "#BF616A"      # Nord red
    border_color_unfocused: "#4C566A"
    opacity: 1.0
    opacity_inactive: 0.85
    border_radius: 10
    enable_shadows: true
    shadow_size: 12

window-rules:
  - name: spotify
    title: "Spotify"
    decoration_group: media-player
    force_floating: true
  
  - name: vlc
    app_id: "vlc"
    decoration_group: media-player
```

## Finding Application IDs

To find the `app_id` or `title` of a window:

1. Use the IPC command:
```bash
leviathanctl get-clients
```

2. Check the logs when a window opens:
```
[info] Window mapped - app_id='kitty', title='kitty'
```

3. Use Wayland development tools:
```bash
WAYLAND_DEBUG=1 <application> 2>&1 | grep app_id
```

## Troubleshooting

### Decorations Not Applied

- **Check rule order**: The catch-all `app_id: "*"` rule should be last
- **Verify app_id**: Use `leviathanctl get-clients` to check the actual app_id
- **Check logs**: Look for "Matched window rule" and "Applied decoration group" messages
- **Pattern matching**: Ensure wildcard patterns are correct (`"*term*"` not `"*term"`)

### Transparency Not Working

- **Terminal support**: Ensure your terminal supports transparency (most modern terminals do)
- **Compositor backend**: Transparency works in nested Wayland/X11 and on native DRM
- **Opacity values**: Check that `opacity` or `opacity_inactive` is less than 1.0

### Shadows Not Visible

- **Enable shadows**: Set `enable_shadows: true` in the decoration
- **Shadow size**: Increase `shadow_size` if shadows are too subtle
- **Shadow opacity**: Increase `shadow_opacity` (try 0.6 or higher)
- **Shadow offset**: Add `shadow_offset_y: 4` for more visible shadows

## See Also

- [Configuration Guide](/docs/configuration) - Full configuration reference
- [Status Bar](/docs/features/status-bar) - Status bar customization
- [Wallpapers](/docs/features/wallpapers) - Wallpaper configuration
