---
title: "Keybindings"
weight: 3
---

# Keybindings

LeviathanDM uses vim-style keybindings for window management.

{{< hint info >}}
**Press `Super + F1`** to show the built-in keybinding help overlay anytime!
{{< /hint >}}

## Modifier Key

The default modifier key is **Super** (Windows key / Command key).

## Window Management

| Key | Action |
|-----|--------|
| `Super + Return` | Launch terminal |
| `Super + Shift + Q` | Close focused window |
| `Super + J` | Focus next window |
| `Super + K` | Focus previous window |
| `Super + Shift + J` | Swap with next window |
| `Super + Shift + K` | Swap with previous window |

## Workspace/Tag Switching

| Key | Action |
|-----|--------|
| `Super + 1-9` | Switch to tag 1-9 |
| `Super + Shift + 1-9` | Move window to tag 1-9 |

## Layout Control

| Key | Action |
|-----|--------|
| `Super + T` | Tiling layout (master-stack) |
| `Super + M` | Monocle layout (fullscreen) |
| `Super + G` | Grid layout |
| `Super + H` | Decrease master width |
| `Super + L` | Increase master width |

## System

| Key | Action |
|-----|--------|
| `Super + F1` | Show keybinding help |
| `Super + Shift + E` | Exit compositor |

## Mouse Bindings

| Mouse | Action |
|-------|--------|
| `Super + Left Click` | Move window |
| `Super + Right Click` | Resize window |
| `Click on window` | Focus window |

## Per-Screen Tags

LeviathanDM uses per-screen tags (AwesomeWM model):
- Each monitor has independent tags
- Windows stay on their screen
- Focus can move between screens

## Customization

{{< hint warning >}}
Custom keybindings are not yet configurable via YAML. Coming soon!
{{< /hint >}}

For now, keybindings are defined in:
- `src/KeyBindings.cpp`
- Recompile to change

## Next Steps

- [Tiling Layouts]({{< relref "/docs/features/layouts" >}})
- [Status Bar]({{< relref "/docs/features/status-bar" >}})
