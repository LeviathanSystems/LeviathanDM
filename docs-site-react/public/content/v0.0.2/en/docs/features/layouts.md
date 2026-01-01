---
title: "Tiling Layouts"
weight: 3
---

# Tiling Layouts

LeviathanDM supports multiple tiling layouts for window management.

## Available Layouts

### Master-Stack (dwm-style)

The default layout with a master area and stacked secondary windows.

```
┌─────────┬──────┐
│         │  2   │
│    1    ├──────┤
│ Master  │  3   │
│         ├──────┤
│         │  4   │
└─────────┴──────┘
```

**Keybindings:**
- `Super + T`: Switch to master-stack
- `Super + H`: Decrease master width
- `Super + L`: Increase master width

**Configuration:**
```yaml
layouts:
  default: "master-stack"
  master_width_ratio: 0.55  # 55% for master
```

### Monocle (Fullscreen)

Each window takes the full screen, stacked.

```
┌──────────────┐
│              │
│      1       │
│  (fullscreen)│
│              │
└──────────────┘
```

**Keybindings:**
- `Super + M`: Switch to monocle

**Use Cases:**
- Maximum screen real estate
- Single-app focus mode
- Presentation mode

### Grid Layout

Windows arranged in a grid pattern.

```
┌──────┬──────┐
│  1   │  2   │
├──────┼──────┤
│  3   │  4   │
└──────┴──────┘
```

**Keybindings:**
- `Super + G`: Switch to grid

**Use Cases:**
- Multiple similar windows
- Monitoring multiple terminals
- Comparing documents

## Layout Switching

Switch layouts with:
- `Super + T` - Master-stack (tiling)
- `Super + M` - Monocle (fullscreen)
- `Super + G` - Grid

{{< hint info >}}
Layout state is per-tag, so each workspace can have a different layout!
{{< /hint >}}

## Master Area Control

In master-stack layout:
- `Super + H`: Make master area narrower
- `Super + L`: Make master area wider

## Per-Tag Layouts

{{< hint warning >}}
Per-tag layout persistence is planned but not yet implemented.
{{< /hint >}}

Future: Each tag will remember its layout preference.

## Floating Windows

{{< hint warning >}}
Floating window mode is planned but not yet implemented.
{{< /hint >}}

Planned features:
- `Super + Space`: Toggle floating for focused window
- Drag windows with `Super + Left Click`
- Resize windows with `Super + Right Click`

## Configuration

```yaml
layouts:
  # Default layout for new tags
  default: "master-stack"
  
  # Master-stack settings
  master_width_ratio: 0.55
  
  # Grid settings (future)
  grid_columns: 2
  
  # Gaps and borders
  gap_size: 10
  border_width: 2
```

## See Also

- [Keybindings]({{< relref "/docs/getting-started/keybindings" >}})
- [Configuration]({{< relref "/docs/getting-started/configuration" >}})
