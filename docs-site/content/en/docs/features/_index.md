---
bookFlatSection: false
bookCollapseSection: false
weight: 2
title: "Features"
---

# Features

Explore LeviathanDM's built-in features and capabilities.

## Available Features

### [Wallpapers](wallpapers)
{{< version-warning type="new" version="v0.1.0" >}}
Built-in wallpaper system with support for static images, rotating slideshows, and per-monitor configuration.
{{< /version-warning >}}

### [Notification Daemon](notifications)
Native Desktop Notifications spec implementation with no external dependencies required.

### [Status Bar](status-bar)
Highly customizable status bar with widget plugin system for displaying system information.

{{< hint info >}}
**New in v0.0.3**: Widget-based popovers! Build complex popup layouts using composable widgets (VBox, HBox, Label). See the [Widget System Guide]({{< relref "/docs/development/widget-system" >}}).
{{< /hint >}}

### [Tiling Layouts](layouts)
Multiple tiling algorithms including Master-Stack, Monocle, and Grid layouts with dynamic adjustments.
