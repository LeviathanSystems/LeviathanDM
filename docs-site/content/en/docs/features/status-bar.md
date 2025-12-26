---
title: "Status Bar"
weight: 2
---

# Status Bar

LeviathanDM features a customizable status bar with a plugin-based widget system.

## Features

✅ Multiple status bars per screen  
✅ Flexible positioning (top, bottom)  
✅ Plugin-based widget system  
✅ Declarative YAML configuration  
✅ Thread-safe widget updates  
✅ DBus integration for system info  

## Configuration

Status bars are configured in `leviathan.yaml`:

```yaml
status_bars:
  - screen: 0
    position: "top"
    height: 30
    background: "#2E3440"
    widgets:
      - type: "tags"
        position: "left"
        config:
          focused_color: "#88C0D0"
          unfocused_color: "#4C566A"
          
      - type: "clock"
        position: "right"
        config:
          format: "%H:%M:%S"
          update_interval: 1
          
      - type: "battery"
        position: "right"
        config:
          show_percentage: true
          low_threshold: 20
          critical_threshold: 10
```

## Built-in Widgets

### Tags Widget
Displays workspace/tag indicators.

**Config:**
- `focused_color`: Color for active tag
- `unfocused_color`: Color for inactive tags

### Clock Widget
Displays current time and date.

**Config:**
- `format`: strftime format string (default: `%H:%M`)
- `update_interval`: Seconds between updates (default: 60)

### Battery Widget
Shows battery status with popover for details.

**Config:**
- `show_percentage`: Show battery percentage
- `low_threshold`: Show warning below this %
- `critical_threshold`: Critical battery level %

## Widget Plugins

Widgets are dynamically loaded from:
- `/usr/local/lib/leviathan/plugins/`
- `~/.local/lib/leviathan/plugins/`

### Available Plugins

- **clock-widget**: Time and date display
- **battery-widget**: Battery monitoring with UPower
- **tags-widget**: Workspace indicators (built-in)

### Creating Custom Widgets

See [Plugin Development]({{< relref "/docs/development/plugins" >}}) for how to create custom widgets.

## Widget API

Widgets can:
- Render custom Cairo graphics
- Subscribe to DBus signals
- Update periodically
- Show popovers on click
- Access compositor state

## Examples

### Minimal Status Bar

```yaml
status_bars:
  - screen: 0
    position: "top"
    height: 25
    widgets:
      - type: "clock"
```

### Multi-Widget Bar

```yaml
status_bars:
  - screen: 0
    position: "top"
    height: 30
    widgets:
      - type: "tags"
        position: "left"
      - type: "clock"
        position: "center"
      - type: "battery"
        position: "right"
```

### Multi-Monitor Setup

```yaml
status_bars:
  - screen: 0  # Primary monitor
    position: "top"
    widgets:
      - type: "tags"
      - type: "clock"
      
  - screen: 1  # Secondary monitor
    position: "bottom"
    widgets:
      - type: "tags"
```

## See Also

- [Plugin Development]({{< relref "/docs/development/plugins" >}})
- [Configuration]({{< relref "/docs/getting-started/configuration" >}})
