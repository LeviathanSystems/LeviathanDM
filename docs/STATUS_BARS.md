# Status Bar Configuration Guide

## Overview

LeviathanDM supports flexible, per-monitor status bar configuration. You can:
- Define multiple named status bars with different appearances
- Assign different status bars to different monitors
- Use multiple status bars on a single monitor (e.g., top + bottom)
- Position status bars at top, bottom, left, or right edges
- Configure widgets with three-section layout (left, center, right)

## Architecture

### How It Works

```
┌─────────────────────────────────────────────────────┐
│  1. Define Status Bars (status-bars section)       │
│     - Create named bars: "laptop-bar", "desk-bar"  │
│     - Configure position, size, appearance          │
│     - Add widgets to left/center/right sections     │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│  2. Assign to Monitors (monitor-groups section)    │
│     - Reference status bars by name                 │
│     - Each monitor can have 0+ status bars          │
│     - Bars can be reused across monitors            │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│  3. Compositor Creates Status Bars                  │
│     - Finds monitor's assigned status bars          │
│     - Creates StatusBar instances                   │
│     - Reserves space in LayerManager                │
│     - Renders widgets in WorkingArea layer          │
└─────────────────────────────────────────────────────┘
```

## Configuration Format

### 1. Define Status Bars

```yaml
status-bars:
  - name: laptop-bar         # Unique identifier
    position: top            # top, bottom, left, or right
    height: 30               # Height in pixels (for top/bottom bars)
    width: 40                # Width in pixels (for left/right bars)
    
    # Appearance
    background_color: "#2E3440"
    foreground_color: "#D8DEE9"
    font_size: 12
    font_family: "JetBrains Mono"
    
    # Widget sections
    left:
      spacing: 10             # Pixels between widgets
      alignment: left         # left, center, right (for HBox)
      padding: 5              # Edge padding
      widgets:
        - type: workspaces
          active_color: "#88C0D0"
        - type: window-title
    
    center:
      widgets:
        - type: clock
          format: "%H:%M:%S"
    
    right:
      widgets:
        - type: battery
        - type: network
```

### 2. Assign to Monitors

```yaml
monitor-groups:
  - name: Home
    monitors:
      - display: eDP-1
        status-bars:          # List of status bar names
          - laptop-bar
      
      - display: "d:Dell Inc./U2720Q"
        status-bars:
          - external-top      # Different bar
          - external-bottom   # Multiple bars!
```

## Widget Types

### Built-in Widgets

| Type | Description | Common Properties |
|------|-------------|-------------------|
| `workspaces` | Tag/workspace indicators | `active_color`, `inactive_color`, `style` |
| `window-title` | Current window title | `max_length`, `ellipsis` |
| `clock` | Date/time display | `format`, `update_interval` |
| `battery` | Battery status | `show_percentage`, `show_icon` |
| `network` | Network status | `show_name`, `show_icon` |
| `volume` | Volume/audio status | `show_percentage`, `show_icon` |
| `cpu` | CPU usage | `update_interval` |
| `memory` | Memory usage | `update_interval` |
| `label` | Static text | `text`, `color` |
| `system-tray` | System tray icons | - |

### Plugin Widgets

Load custom widgets from plugins:

```yaml
status-bars:
  - name: my-bar
    left:
      widgets:
        - type: ClockWidget       # From loaded plugin
          format: "%H:%M"
          custom_property: value
```

## Position Options

### Horizontal Bars (Top/Bottom)

```
┌────────────────────────────────────────┐
│  [left widgets] [center] [right]       │  ← Top bar
├────────────────────────────────────────┤
│                                        │
│          Window Content                │
│                                        │
├────────────────────────────────────────┤
│  [left widgets] [center] [right]       │  ← Bottom bar
└────────────────────────────────────────┘
```

- **Position**: `top` or `bottom`
- **Size**: `height` property (pixels)
- **Sections**: `left`, `center`, `right` (horizontal layout)

### Vertical Bars (Left/Right)

```
┌──┬────────────────────────────────┬──┐
│ T│                                │ T│
│ o│                                │ o│
│ p│         Window Content         │ p│
│  │                                │  │
│ C│                                │ C│
│ e│                                │ e│
│ n│                                │ n│
│ t│                                │ t│
│ e│                                │ e│
│ r│                                │ r│
│  │                                │  │
│ B│                                │ B│
│ o│                                │ o│
│ t│                                │ t│
└──┴────────────────────────────────┴──┘
 ↑                                  ↑
Left bar                        Right bar
```

- **Position**: `left` or `right`
- **Size**: `width` property (pixels)
- **Sections**: `left` (top), `center` (middle), `right` (bottom)

## Reserved Space

Status bars automatically reserve space in the LayerManager:

```cpp
// Example: Top bar reserves 30px
ReservedSpace {
    .top = 30,
    .bottom = 0,
    .left = 0,
    .right = 0
};

// Multiple bars on one monitor
ReservedSpace {
    .top = 30,      // Top bar
    .bottom = 25,   // Bottom bar
    .left = 40,     // Left bar
    .right = 0
};
```

The remaining area is used for tiling windows.

## Examples

### Example 1: Simple Setup

```yaml
status-bars:
  - name: main
    position: top
    height: 28
    left:
      widgets:
        - type: workspaces
    center:
      widgets:
        - type: clock
          format: "%H:%M"
    right:
      widgets:
        - type: battery

monitor-groups:
  - name: Default
    monitors:
      - display: eDP-1
        status-bar: main  # Single bar shorthand
```

### Example 2: Multi-Monitor Different Bars

```yaml
status-bars:
  - name: laptop-full
    position: top
    height: 30
    # ... detailed widgets ...
  
  - name: monitor-minimal
    position: bottom
    height: 20
    # ... minimal widgets ...

monitor-groups:
  - name: Docked
    monitors:
      - display: eDP-1
        status-bars: [laptop-full]
      
      - display: HDMI-A-1
        status-bars: [monitor-minimal]
```

### Example 3: Multiple Bars Per Monitor

```yaml
status-bars:
  - name: top-bar
    position: top
    height: 28
    left:
      widgets:
        - type: workspaces
    center:
      widgets:
        - type: window-title
  
  - name: bottom-bar
    position: bottom
    height: 24
    left:
      widgets:
        - type: cpu
        - type: memory
    right:
      widgets:
        - type: network

monitor-groups:
  - name: Default
    monitors:
      - display: eDP-1
        status-bars:
          - top-bar
          - bottom-bar  # Both bars!
```

### Example 4: No Status Bars

```yaml
monitor-groups:
  - name: Presentation
    monitors:
      - display: HDMI-A-1
        # No status-bars field = no bars!
```

### Example 5: Vertical Bar

```yaml
status-bars:
  - name: side-bar
    position: left
    width: 40
    left:  # "left" = top of vertical bar
      alignment: top
      widgets:
        - type: workspaces
          orientation: vertical
    center:
      widgets:
        - type: clock
          format: "%H\n%M"
          orientation: vertical

monitor-groups:
  - name: Ultrawide
    monitors:
      - display: "d:LG/34GN850"
        status-bars: [side-bar]
```

## Widget Configuration

Each widget type accepts different properties passed as key-value pairs:

```yaml
widgets:
  - type: clock
    format: "%A, %B %d  %H:%M:%S"
    update_interval: 1
    color: "#88C0D0"
    font_size: 14
```

Properties are:
- Parsed from YAML as strings
- Passed to widget constructor
- Interpreted by the widget implementation
- Can be plugin-specific

## Dynamic Monitor Switching

When monitors connect/disconnect:

1. Compositor detects change
2. Finds matching monitor group
3. Reads `status-bars` list for each monitor
4. Creates/destroys StatusBar instances
5. Updates LayerManager reserved space
6. Retiles windows in new usable area

## Implementation Notes

### Data Structures

```cpp
struct StatusBarConfig {
    string name;
    Position position;  // Top, Bottom, Left, Right
    int height/width;
    string colors, fonts;
    ContainerConfig left, center, right;
};

struct MonitorConfig {
    string identifier;
    vector<string> status_bars;  // Status bar names
    // ...
};
```

### StatusBar Creation Flow

```
1. OnNewOutput() triggered
   ↓
2. Find MonitorConfig for this output
   ↓
3. Get status_bars list
   ↓
4. For each bar name:
   a. Config().status_bars.FindByName(name)
   b. Create StatusBar(config, layer_manager, output_width/height)
   c. StatusBar reserves space in LayerManager
   d. StatusBar creates widgets
   e. Add to output->status_bars list
   ↓
5. TileViews() uses updated usable area
```

### Reserved Space Calculation

```cpp
// Multiple status bars accumulate reserved space
ReservedSpace total = {0, 0, 0, 0};

for (auto* bar : output->status_bars) {
    switch (bar->position) {
        case Top:    total.top += bar->height; break;
        case Bottom: total.bottom += bar->height; break;
        case Left:   total.left += bar->width; break;
        case Right:  total.right += bar->width; break;
    }
}

layer_manager->SetReservedSpace(total);
```

## See Also

- `config/status-bars-example.yaml` - Complete configuration examples
- `ARCHITECTURE.md` - LayerManager and reserved space details
- `include/ui/StatusBar.hpp` - StatusBar implementation
- `include/config/ConfigParser.hpp` - Configuration parsing

---

**Last Updated**: December 19, 2025
