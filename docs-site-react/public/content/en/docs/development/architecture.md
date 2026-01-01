---
title: "Architecture"
weight: 1
---

# Architecture

LeviathanDM follows a layered architecture design.

## Core Layers

### Wayland Layer
The Wayland layer handles all compositor functionality using wlroots.

### Core Layer  
The core layer provides abstractions for screens, tags, and clients.

### Layout Layer
The layout layer handles window tiling and positioning.

### UI Layer
The UI layer provides status bars, widgets, and the notification daemon.

For more details, see the [ARCHITECTURE.md](https://github.com/LeviathanSystems/LeviathanDM/blob/master/ARCHITECTURE.md) file in the repository.
