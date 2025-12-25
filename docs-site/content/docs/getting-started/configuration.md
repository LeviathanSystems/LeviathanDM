---
title: "Configuration"
weight: 2
---

# Configuration

LeviathanDM uses YAML for configuration.

## Configuration File

The configuration file is located at:
```
~/.config/leviathan/leviathan.yaml
```

If not found, LeviathanDM looks in:
1. `$XDG_CONFIG_HOME/leviathan/leviathan.yaml`
2. `~/.config/leviathan/leviathan.yaml`
3. `./config/leviathan.yaml` (project directory)

## Example Configuration

```yaml
general:
  gap_size: 10
  border_width: 2
  border_color_focused: "#ff0000"
  border_color_unfocused: "#3B4252"
  
layouts:
  default: "master-stack"
  master_width_ratio: 0.55
  
status_bars:
  - screen: 0
    position: "top"
    height: 30
    widgets:
      - type: "tags"
        config:
          focused_color: "#88C0D0"
      - type: "clock"
        config:
          format: "%H:%M"
      - type: "battery"
```

## Configuration Sections

### General

- `gap_size`: Pixels between windows (default: 10)
- `border_width`: Border width in pixels (default: 2)
- `border_color_focused`: Focused window border color (hex)
- `border_color_unfocused`: Unfocused window border color (hex)

### Layouts

- `default`: Default layout mode (`master-stack`, `monocle`, `grid`)
- `master_width_ratio`: Width ratio for master area (0.0-1.0)

### Status Bars

See [Status Bar Documentation]({{< relref "/docs/features/status-bar" >}}) for details.

### Keybindings

See [Keybindings Documentation]({{< relref "/docs/getting-started/keybindings" >}}) for details.

## Hot Reload

{{< hint warning >}}
Hot reload is not yet implemented. Restart the compositor after changing configuration.
{{< /hint >}}

## Examples

Example configurations are in the `config/` directory:

- `config/leviathan.yaml` - Main example
- `config/status-bars-example.yaml` - Status bar examples
- `config/monitor-groups-example.yaml` - Multi-monitor setup

## Next Steps

- [Keybindings]({{< relref "/docs/getting-started/keybindings" >}})
- [Status Bar]({{< relref "/docs/features/status-bar" >}})
