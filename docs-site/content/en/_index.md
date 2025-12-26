---
title: "LeviathanDM"
type: docs
bookToc: false
---

# LeviathanDM

A lightweight, customizable tiling Wayland compositor built with wlroots and C++.

{{< hint info >}}
**Quick Start:** Jump to [Building]({{< relref "/docs/getting-started/building" >}}) to get started!
{{< /hint >}}

## Features

- **Multiple Tiling Layouts**: Master-Stack, Monocle, Grid
- **9 Workspaces**: Quick workspace switching
- **Per-Screen Tags**: Independent workspaces per monitor (AwesomeWM model)
- **Built-in Wallpaper System**: Static images and rotating slideshows
- **Built-in Notification Daemon**: Full Desktop Notifications spec support
- **Customizable Status Bar**: Widget-based with plugin support
- **Customizable**: Colors, gaps, borders, keybindings
- **Wayland Native**: Modern protocol with wlroots
- **Vim-like Keybindings**: Familiar hjkl navigation
- **Built-in Help**: Press `Super + F1` for keybinding overlay

## Why LeviathanDM?

Unlike other window managers:

- **AwesomeWM**: We provide a built-in notification daemon, no external tools needed
- **i3/Sway**: Native Wayland from the ground up, not a port
- **dwm**: Dynamic configuration without recompilation (coming soon)

## Documentation

{{< columns >}}

### Getting Started
- [Building from Source]({{< relref "/docs/getting-started/building" >}})
- [Configuration]({{< relref "/docs/getting-started/configuration" >}})
- [Keybindings]({{< relref "/docs/getting-started/keybindings" >}})

<--->

### Features
- [Wallpapers]({{< relref "/docs/features/wallpapers" >}})
- [Notification Daemon]({{< relref "/docs/features/notifications" >}})
- [Status Bar]({{< relref "/docs/features/status-bar" >}})
- [Tiling Layouts]({{< relref "/docs/features/layouts" >}})

<--->

### Development
- [Architecture]({{< relref "/docs/development/architecture" >}})
- [Plugin System]({{< relref "/docs/development/plugins" >}})
- [Contributing]({{< relref "/docs/development/contributing" >}})

{{< /columns >}}

## Quick Example

```bash
# Build
./build.sh

# Run in nested session
cage build/leviathan

# Or from TTY
./build/leviathan
```

## Screenshots

{{< hint warning >}}
Screenshots coming soon!
{{< /hint >}}

## Community

- **GitHub**: [LeviathanSystems/LeviathanDM](https://github.com/LeviathanSystems/LeviathanDM)
- **Issues**: Report bugs and request features
- **Discussions**: Ask questions and share ideas

## License

See the [LICENSE](https://github.com/LeviathanSystems/LeviathanDM/blob/master/LICENSE) file for details.
