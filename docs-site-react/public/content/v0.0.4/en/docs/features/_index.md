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
Built-in wallpaper system with support for static images, rotating slideshows, and per-monitor configuration.

### [Notification Daemon](notifications)
Native Desktop Notifications spec implementation with no external dependencies required.

### [Status Bar](status-bar)
Highly customizable status bar with widget plugin system for displaying system information.

Widget-based popovers build complex popup layouts using composable widgets (VBox, HBox, Label). See the [Widget System Guide](/docs/development/widget-system).

### [Tiling Layouts](layouts)
Multiple tiling algorithms including Master-Stack, Monocle, and Grid layouts with dynamic adjustments.

### Modal System & Scrolling
**New in v0.0.4**  
Full modal dialog system with:
- ScrollView widget for scrollable content
- Hierarchical coordinate system for proper positioning
- Keyboard and mouse wheel scroll support
- Smooth rendering with cairo transformations

See the [Architecture Guide](/docs/development/architecture) for technical details.
