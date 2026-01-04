---
bookFlatSection: false
bookCollapseSection: false
weight: 2
title: "Features"
---

# Features

Explore LeviathanDM's built-in features and capabilities.

## Available Features

### [XWayland Support](xwayland)
**New in v0.0.5**  
Full X11 application support enabling legacy applications to run seamlessly alongside native Wayland clients. Chrome, Firefox, and other X11 apps now work without crashes.

### [Wallpapers](wallpapers)
Built-in wallpaper system with support for static images, rotating slideshows, and per-monitor configuration.

### [Notification Daemon](notifications)
Native Desktop Notifications spec implementation with no external dependencies required.

### [Status Bar](status-bar)
Highly customizable status bar with widget plugin system for displaying system information.

### [MenuBar & Application Launcher](menubar)
**Enhanced in v0.0.5**  
Desktop application integration with automatic .desktop file scanning, custom commands, and bookmark support. Launch any installed application with a single click.

### [Tabs Bar](tabs)
**New in v0.0.5 (In Development)**  
Window organization with tabbed interface. Group related windows together and switch between them efficiently.

Widget-based popovers build complex popup layouts using composable widgets (VBox, HBox, Label). See the [Widget System Guide](/docs/development/widget-system).

### [Tiling Layouts](layouts)
Multiple tiling algorithms including Master-Stack, Monocle, and Grid layouts with dynamic adjustments. Now supports both Wayland and X11 windows with intelligent protocol detection.

### Modal System & Scrolling
**Introduced in v0.0.4**  
Full modal dialog system with:
- ScrollView widget for scrollable content
- Hierarchical coordinate system for proper positioning
- Keyboard and mouse wheel scroll support
- Smooth rendering with cairo transformations

See the [Architecture Guide](/docs/development/architecture) for technical details.
