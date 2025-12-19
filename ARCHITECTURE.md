# LeviathanDM Architecture

LeviathanDM is a Wayland compositor built on wlroots 0.19, designed for tiling window management with dynamic multi-monitor support.

## Table of Contents
- [Core Concepts](#core-concepts)
- [Seat System](#seat-system)
- [Screen Management](#screen-management)
- [Layer System](#layer-system)
- [Tag-Based Workspaces](#tag-based-workspaces)
- [Tiling Layout Engine](#tiling-layout-engine)
- [Monitor Groups](#monitor-groups)
- [Status Bars](#status-bars)
- [Widget Plugin System](#widget-plugin-system)

---

## Core Concepts

LeviathanDM is built around several key architectural concepts:

1. **Seat**: Represents a set of input devices (keyboard, mouse) and manages user interaction
2. **Screens**: Physical display outputs with per-output rendering layers
3. **Layers**: Z-ordered rendering layers for different types of content
4. **Tags**: Virtual workspaces that can be assigned to windows
5. **Clients**: Application windows managed by the compositor

---

## Seat System

### What is a Seat?

A **seat** represents a logical grouping of input devices and the focus state for those devices. In most desktop scenarios, there is one seat controlling one keyboard and one mouse.

### Core::Seat (`include/core/Seat.hpp`)

The seat manages:
- **Active Tag**: Which virtual workspace is currently visible
- **Screens**: List of connected display outputs
- **Focus Management**: Which window receives keyboard/mouse input
- **Clients**: All windows currently managed by the compositor

```cpp
class Seat {
    void AddScreen(Screen* screen);
    void SetActiveTag(Tag* tag);
    Tag* GetActiveTag();
    void AddClient(Client* client);
    // ...
};
```

### Key Responsibilities

1. **Tag Switching**: When you switch tags, the seat handles showing/hiding clients
2. **Focus Tracking**: Maintains which window has keyboard focus
3. **Client Organization**: Keeps track of all windows and their tag assignments

---

## Screen Management

### What is a Screen?

A **screen** represents a physical display output (monitor). Each screen has:
- A `wlr_output` (wlroots output object)
- A unique name (e.g., "eDP-1", "HDMI-A-1")
- Resolution, refresh rate, and position information

### Core::Screen (`include/core/Screen.hpp`)

```cpp
class Screen {
    Screen(struct wlr_output* output);
    
    const std::string& GetName() const;
    int GetWidth() const;
    int GetHeight() const;
    struct wlr_output* GetWlrOutput();
};
```

### Per-Output Architecture

Each screen has its own:
- **LayerManager**: Manages the 3-layer rendering hierarchy
- **Scene Output**: wlroots scene graph output for rendering
- **Layout Position**: Position in the multi-monitor layout

---

## Layer System

### Layer Hierarchy

LeviathanDM uses a **3-layer rendering system** for each output. Layers are rendered bottom-to-top:

```
┌─────────────────────────────────┐
│         Top Layer               │  ← Overlays, notifications, scratchpads
├─────────────────────────────────┤
│      Working Area Layer         │  ← Regular application windows, status bars
├─────────────────────────────────┤
│      Background Layer           │  ← Wallpaper, solid colors
└─────────────────────────────────┘
```

### Layer Definitions (`include/wayland/LayerManager.hpp`)

```cpp
enum class Layer {
    Background = 0,   // Bottom: Wallpaper, background elements
    WorkingArea = 1,  // Middle: Application windows and bars
    Top = 2,          // Top: Floating overlays, notifications
    COUNT = 3
};
```

### LayerManager

Each screen has a `LayerManager` that provides:

#### 1. Layer Access
```cpp
wlr_scene_tree* GetLayer(Layer layer);
wlr_scene_tree* GetBackgroundLayer();
wlr_scene_tree* GetWorkingAreaLayer();
wlr_scene_tree* GetTopLayer();
```

#### 2. Reserved Space Management
```cpp
struct ReservedSpace {
    uint32_t top;     // Pixels reserved at top (e.g., status bar)
    uint32_t bottom;  // Pixels reserved at bottom (e.g., dock)
    uint32_t left;    // Pixels reserved at left
    uint32_t right;   // Pixels reserved at right
};

void SetReservedSpace(const ReservedSpace& space);
```

#### 3. Usable Area Calculation
```cpp
struct UsableArea {
    int32_t x, y;           // Top-left corner
    uint32_t width, height; // Available space for windows
};

UsableArea CalculateUsableArea(int32_t output_x, int32_t output_y,
                               uint32_t output_width, uint32_t output_height);
```

#### 4. Window Tiling
```cpp
void TileViews(std::vector<View*>& views, 
               Core::Tag* tag,
               TilingLayout* layout_engine);
```

### Layer Usage Examples

**Background Layer:**
- Wallpaper images
- Solid color backgrounds
- Desktop icons (if implemented)

**Working Area Layer:**
- Regular application windows (tiled or floating)
- Status bars and panels
- Docks

**Top Layer:**
- Scratchpad windows
- Notifications
- Pop-up menus and tooltips
- Always-on-top windows

### Reserved Space

When a status bar or dock reserves space, it reduces the usable area for tiling windows:

```
Without reserved space:        With 30px top reserved space:
┌────────────────┐            ┌────────────────┐
│                │            │  Status Bar    │ ← 30px reserved
│                │            ├────────────────┤
│   Tiled        │            │                │
│   Windows      │            │   Tiled        │
│   (Full)       │            │   Windows      │
│                │            │   (Reduced)    │
└────────────────┘            └────────────────┘
 1280x720 usable               1280x690 usable
```

---

## Tag-Based Workspaces

### What are Tags?

**Tags** are LeviathanDM's implementation of virtual workspaces/desktops. Unlike traditional workspaces:
- Windows can have multiple tags simultaneously
- You can view multiple tags at once (tag union)
- Tags are independent of monitors

### Core::Tag (`include/core/Tag.hpp`)

```cpp
class Tag {
    Tag(const std::string& name);
    
    // Client management
    void AddClient(Client* client);
    void RemoveClient(Client* client);
    std::vector<Client*> GetClients();
    
    // Visibility
    void Show();
    void Hide();
    bool IsVisible() const;
    
    // Layout configuration
    void SetLayout(LayoutType layout);
    LayoutType GetLayout() const;
    void SetMasterCount(int count);
    void SetMasterRatio(float ratio);
};
```

### Tag Lifecycle

1. **Creation**: Tags are created from config file (usually named "1", "2", "3", etc.)
2. **Assignment**: Windows are assigned to tags when created or via keybindings
3. **Activation**: Setting a tag as active shows all its windows
4. **Switching**: Switching tags hides old tag's windows, shows new tag's windows

### Example Tag Usage

```yaml
# config.yaml
tags:
  - name: "1"
    layout: master-stack
    master_count: 1
    master_ratio: 0.6
  - name: "2"
    layout: grid
  - name: "3"
    layout: monocle
```

### Tag Properties

Each tag maintains:
- **Name**: User-visible identifier ("1", "web", "code", etc.)
- **Layout Type**: master-stack, grid, or monocle
- **Master Count**: How many windows go in master area
- **Master Ratio**: Size ratio between master and stack (0.0 - 1.0)
- **Clients**: List of windows assigned to this tag

---

## Tiling Layout Engine

### TilingLayout (`include/layout/TilingLayout.hpp`)

The layout engine calculates window positions and sizes based on the active tag's layout configuration.

### Layout Types

#### 1. Master-Stack Layout
```
┌──────────┬─────┐
│          │  2  │
│          ├─────┤
│    1     │  3  │
│  Master  ├─────┤
│          │  4  │
│          ├─────┤
│          │  5  │
└──────────┴─────┘
```

- First N windows go in master area (left)
- Remaining windows stack vertically (right)
- Master ratio determines master width (default 0.6 = 60%)

#### 2. Grid Layout
```
┌─────┬─────┬─────┐
│  1  │  2  │  3  │
├─────┼─────┼─────┤
│  4  │  5  │  6  │
└─────┴─────┴─────┘
```

- Windows arranged in a grid
- Automatically calculates rows × columns
- All windows get equal space

#### 3. Monocle Layout
```
┌───────────────────┐
│                   │
│         1         │
│    (Fullscreen)   │
│                   │
└───────────────────┘
```

- One window fills entire workspace
- Other windows hidden behind
- Cycle through with focus keybindings

### Layout Algorithm Flow

1. **Tag requests layout**: When windows are added/removed/resized
2. **Collect tileable views**: Filter out floating, fullscreen, unmapped windows
3. **Get usable area**: LayerManager calculates area minus reserved space
4. **Apply layout algorithm**: Calculate x, y, width, height for each window
5. **Set positions**: Update scene graph and send configure to windows

```cpp
void LayerManager::TileViews(std::vector<View*>& views, 
                             Core::Tag* tag,
                             TilingLayout* layout_engine) {
    // Calculate usable workspace area (minus reserved space)
    auto workspace = CalculateUsableArea(0, 0, output_->width, output_->height);
    
    // Apply layout algorithm
    switch (tag->GetLayout()) {
        case LayoutType::MASTER_STACK:
            layout_engine->ApplyMasterStack(views, master_count, master_ratio,
                                           workspace.width, workspace.height, gap);
            break;
        // ...
    }
}
```

---

## Monitor Groups

### Dynamic Multi-Monitor Configuration

Monitor groups allow you to define different monitor layouts for different scenarios (work, home, presentations, etc.).

### Configuration Format

```yaml
monitor-groups:
  - name: Default
    monitors:
      - identifier: eDP-1  # Laptop screen
        position: 0x0
        mode: 1920x1080@60
        scale: 1.0

  - name: Home
    monitors:
      - identifier: eDP-1
        position: 1920x0
        mode: 1920x1080@60
      - identifier: "d:Dell Inc./U2720Q"  # External monitor by description
        position: 0x0
        mode: 3840x2160@60
        scale: 1.5

  - name: Office
    monitors:
      - identifier: eDP-1
        position: 3840x0
      - identifier: "m:Dell Inc./P2419H"  # Match by make/model
        position: 0x0
      - identifier: "m:Dell Inc./P2419H"
        position: 1920x0
```

### Identifier Types

1. **Direct name**: `eDP-1`, `HDMI-A-1` - Exact output name
2. **Description prefix**: `d:Dell Inc./U2720Q` - Match by description
3. **Make/Model prefix**: `m:Dell Inc./U2720Q` - Match by manufacturer and model

### Automatic Application

When outputs connect/disconnect, the compositor:
1. Detects connected outputs
2. Finds best matching monitor group
3. Applies positions, modes, scales, and transforms
4. Falls back to "Default" group if no match

---

## Status Bars

### Per-Monitor Status Bar Configuration

Status bars in LeviathanDM are highly flexible and can be configured per-monitor with multiple bars per screen.

### Key Features

1. **Named Status Bars**: Define status bars with unique names
2. **Multiple Positions**: top, bottom, left, right
3. **Per-Monitor Assignment**: Different monitors get different bars
4. **Multiple Bars Per Monitor**: A monitor can have top + bottom bars simultaneously
5. **Reserved Space**: Bars automatically reserve space from tiling area
6. **Widget System**: Three-section layout (left, center, right) with plugin support

### Configuration Structure

```yaml
# Define status bars
status-bars:
  - name: laptop-bar
    position: top
    height: 30
    background_color: "#2E3440"
    foreground_color: "#D8DEE9"
    
    left:
      widgets:
        - type: workspaces
        - type: window-title
    center:
      widgets:
        - type: clock
          format: "%H:%M"
    right:
      widgets:
        - type: battery

  - name: external-bottom
    position: bottom
    height: 25
    # ...

# Assign to monitors
monitor-groups:
  - name: Home
    monitors:
      - display: eDP-1
        status-bars:
          - laptop-bar
      
      - display: HDMI-A-1
        status-bars:
          - external-bottom
          - external-top  # Multiple bars!
```

### Reserved Space Integration

When status bars are created, they register with the LayerManager:

```cpp
// Example: Top bar (30px) + Bottom bar (25px)
ReservedSpace {
    .top = 30,
    .bottom = 25,
    .left = 0,
    .right = 0
};

// Usable area for tiling windows is reduced by 55px
```

### Status Bar Lifecycle

```
1. Output connects
   ↓
2. Find MonitorConfig
   ↓
3. For each bar in status_bars list:
   a. Lookup StatusBarConfig by name
   b. Create StatusBar instance
   c. Reserve space in LayerManager
   d. Create widget instances
   e. Render in WorkingArea layer
   ↓
4. TileViews() uses reduced usable area
```

### Widget Sections

Each status bar has three sections:

**Horizontal Bars (top/bottom):**
- `left`: Widgets aligned to left edge
- `center`: Widgets centered
- `right`: Widgets aligned to right edge

**Vertical Bars (left/right):**
- `left`: Widgets at top (confusing but consistent naming)
- `center`: Widgets in middle
- `right`: Widgets at bottom

### Example: Multiple Bars

```yaml
status-bars:
  - name: info-bar
    position: top
    height: 28
    left:
      widgets:
        - type: workspaces
    center:
      widgets:
        - type: window-title
  
  - name: system-bar
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
          - info-bar
          - system-bar
```

Result:
```
┌─────────────────────────────────┐
│ [workspaces] [window-title]     │ ← info-bar (28px)
├─────────────────────────────────┤
│                                 │
│      Tiled Windows              │
│      (1920x668 usable)          │
│                                 │
├─────────────────────────────────┤
│ [cpu][mem]         [network]    │ ← system-bar (24px)
└─────────────────────────────────┘
  Total reserved: 52px
```

For detailed status bar documentation, see `docs/STATUS_BARS.md`.

---

## Widget Plugin System

### Architecture

The widget plugin system allows extending the compositor with custom widgets without recompiling.

### Plugin Structure

```
~/.config/leviathan/plugins/
└── clock-widget.so          # Compiled plugin

Plugin API:
- create_widget()            # Factory function
- destroy_widget()           # Cleanup function
- get_metadata()             # Name, version, author
```

### Example Plugin

```cpp
// Clock widget plugin
extern "C" {
    Leviathan::UI::Widget* create_widget(const YAML::Node& config) {
        return new ClockWidget(config);
    }
    
    void destroy_widget(Leviathan::UI::Widget* widget) {
        delete static_cast<ClockWidget*>(widget);
    }
    
    const char* get_metadata() {
        return "ClockWidget|1.0.0|LeviathanDM";
    }
}
```

### Plugin Configuration

```yaml
plugins:
  - name: ClockWidget
    config:
      format: "%H:%M:%S"
      update_interval: 1
      font_size: 12
```

### Plugin Loading Process

1. Scan plugin directories (`~/.config/leviathan/plugins`, `/usr/lib/leviathan/plugins`)
2. Load `.so` files with `dlopen()`
3. Resolve symbols: `create_widget`, `destroy_widget`, `get_metadata`
4. Read plugin configuration from YAML
5. Call `create_widget()` with config
6. Add widget to status bar or other container

---

## Data Flow

### Window Creation Flow

```
1. Wayland client creates window
   ↓
2. XDG surface created (wlroots)
   ↓
3. Server::OnNewXdgToplevel()
   ↓
4. Create View object
   ↓
5. Create Client object
   ↓
6. Assign to active tag (Seat::GetActiveTag())
   ↓
7. Add to tag's client list
   ↓
8. Create scene tree in WorkingArea layer
   ↓
9. Window maps
   ↓
10. Tag::Show() makes scene tree visible
    ↓
11. TileViews() calculates layout
    ↓
12. LayerManager::TileViews() positions window
```

### Tag Switch Flow

```
1. User presses Super+2
   ↓
2. KeyBindings::OnKey() triggered
   ↓
3. Seat::SetActiveTag(tag_2)
   ↓
4. Old tag: Hide all clients' scene trees
   ↓
5. New tag: Show all clients' scene trees
   ↓
6. Server::TileViews()
   ↓
7. LayerManager::TileViews()
   ↓
8. Windows repositioned in layout
```

### Frame Rendering Flow

```
1. Output vsync signal
   ↓
2. Output::HandleFrame() callback
   ↓
3. wlr_scene renders all layers bottom-to-top:
   - Background layer (wallpaper)
   - WorkingArea layer (windows, bars)
   - Top layer (overlays)
   ↓
4. Commit frame to display
```

---

## Key Design Decisions

### 1. Per-Output Layers
Each screen has independent layers, allowing:
- Different wallpapers per monitor
- Independent status bars
- Per-monitor overlays

### 2. Tag Independence
Tags are global, not per-monitor:
- Same tag can show on multiple monitors
- Windows follow tags, not monitors
- Flexible multi-monitor workflows

### 3. Layout in LayerManager
Tiling logic lives in LayerManager because:
- Knows exact usable area (minus reserved space)
- Direct access to output geometry
- Can offset windows for reserved areas

### 4. Plugin System
External plugins allow:
- Custom widgets without forking
- Third-party extensions
- Rapid prototyping

---

## File Organization

```
include/
├── core/
│   ├── Client.hpp         # Window wrapper
│   ├── Screen.hpp         # Monitor wrapper
│   ├── Seat.hpp           # Input/focus manager
│   └── Tag.hpp            # Virtual workspace
├── wayland/
│   ├── Server.hpp         # Main compositor
│   ├── Output.hpp         # Output wrapper
│   ├── LayerManager.hpp   # 3-layer system
│   └── View.hpp           # XDG surface wrapper
├── layout/
│   └── TilingLayout.hpp   # Layout algorithms
├── config/
│   └── ConfigParser.hpp   # YAML configuration
└── ui/
    ├── Widget.hpp         # Base widget class
    └── StatusBar.hpp      # Status bar

src/
├── core/
│   ├── Client.cpp
│   ├── Screen.cpp
│   ├── Seat.cpp
│   └── Tag.cpp
├── wayland/
│   ├── Server.cpp
│   ├── Output.cpp
│   ├── LayerManager.cpp
│   └── View.cpp
├── layout/
│   └── TilingLayout.cpp
└── ui/
    ├── Widget.cpp
    └── StatusBar.cpp
```

---

## Configuration Example

Complete configuration showing all concepts:

```yaml
# Seat configuration
general:
  gap_size: 5
  border_width: 2
  mouse_speed: 0.5
  terminal: alacritty

# Tags (virtual workspaces)
tags:
  - name: "1"
    layout: master-stack
    master_count: 1
    master_ratio: 0.6
  - name: "2"
    layout: grid
  - name: "3"
    layout: monocle

# Monitor groups
monitor-groups:
  - name: Default
    monitors:
      - identifier: eDP-1
        position: 0x0
        mode: 1920x1080@60

  - name: Docked
    monitors:
      - identifier: eDP-1
        position: 2560x0
      - identifier: "d:Dell Inc./U2515H"
        position: 0x0
        mode: 2560x1440@60

# Widget plugins
plugins:
  - name: ClockWidget
    config:
      format: "%H:%M"
      font_size: 12

# Keybindings
keybindings:
  - key: "Super+Return"
    action: spawn_terminal
  - key: "Super+1"
    action: view_tag
    tag: "1"
  - key: "Super+Shift+C"
    action: close_client
```

---

## Further Reading

- **wlroots Documentation**: https://gitlab.freedesktop.org/wlroots/wlroots
- **Wayland Protocol**: https://wayland.freedesktop.org/
- **Scene Graph API**: Used for efficient rendering in wlroots 0.19+

---

**Last Updated**: December 18, 2025  
**Version**: 0.1.0
