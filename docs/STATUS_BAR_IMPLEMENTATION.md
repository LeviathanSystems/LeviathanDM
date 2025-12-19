# Status Bar System Implementation Summary

## What We Built

A comprehensive, flexible per-monitor status bar configuration system for LeviathanDM.

## Key Features

### 1. Named Status Bars
- Define status bars with unique identifiers (e.g., "laptop-bar", "monitor-top")
- Reuse bar configurations across multiple monitors
- Each bar has independent appearance and widget configuration

### 2. Flexible Positioning
- **Top**: Traditional top bar
- **Bottom**: Bottom status bar or dock
- **Left**: Vertical bar on left edge
- **Right**: Vertical bar on right edge

### 3. Per-Monitor Assignment
```yaml
monitor-groups:
  - name: Home
    monitors:
      - display: eDP-1
        status-bars: [laptop-bar]
      
      - display: HDMI-A-1
        status-bars: [external-top, external-bottom]  # Multiple!
```

### 4. Multiple Bars Per Monitor
A single monitor can have multiple status bars simultaneously:
- Top + Bottom bars
- Left + Right bars
- Any combination of positions
- Each bar reserves space independently

### 5. Reserved Space System
- Status bars automatically reserve screen space
- LayerManager calculates usable area for tiling
- Multiple bars accumulate reserved space
- Windows tile in remaining area

### 6. Three-Section Widget Layout
Each bar has three container sections:
- **Left** (or top for vertical bars)
- **Center** (or middle)
- **Right** (or bottom for vertical bars)

Each section can contain multiple widgets with configurable spacing, padding, and alignment.

### 7. Widget Configuration
```yaml
left:
  spacing: 10
  alignment: left
  padding: 5
  widgets:
    - type: workspaces
      active_color: "#88C0D0"
    - type: window-title
      max_length: 50
```

## Architecture

### Data Structures

#### StatusBarConfig
```cpp
struct StatusBarConfig {
    string name;                    // Unique identifier
    Position position;              // Top, Bottom, Left, Right
    int height;                     // For horizontal bars
    int width;                      // For vertical bars
    string background_color;
    string foreground_color;
    int font_size;
    string font_family;
    ContainerConfig left;           // Widget section
    ContainerConfig center;         // Widget section
    ContainerConfig right;          // Widget section
};
```

#### ContainerConfig
```cpp
struct ContainerConfig {
    string type;                    // "hbox" or "vbox"
    vector<WidgetConfig> widgets;
    int spacing;
    string alignment;
    int padding;
};
```

#### WidgetConfig
```cpp
struct WidgetConfig {
    string type;                               // Widget type name
    map<string, string> properties;            // Widget-specific config
};
```

#### MonitorConfig Enhancement
```cpp
struct MonitorConfig {
    string identifier;
    vector<string> status_bars;     // NEW: List of bar names
    optional<pair<int, int>> position;
    optional<string> mode;
    optional<float> scale;
    optional<int> transform;
};
```

### Configuration Flow

```
1. YAML Configuration
   ├─ status-bars: [...]          # Define bars
   └─ monitor-groups:
       └─ monitors:
           └─ status-bars: [...]  # Assign to monitors
          ↓
2. ConfigParser::ParseStatusBars()
   ├─ Parses bar definitions
   ├─ Stores in StatusBarsConfig
   └─ Validates positions, sizes
          ↓
3. ConfigParser::ParseMonitorGroups()
   ├─ Parses monitor configs
   └─ Stores status_bars list per monitor
          ↓
4. Server::OnNewOutput()
   ├─ Finds MonitorConfig
   ├─ For each bar name:
   │   ├─ FindByName() in StatusBarsConfig
   │   ├─ Create StatusBar instance
   │   └─ Reserve space in LayerManager
   └─ TileViews() with updated usable area
```

### Reserved Space Calculation

```cpp
// Example: Top bar (30px) + Bottom bar (25px) + Left bar (40px)
ReservedSpace total = {
    .top = 30,
    .bottom = 25,
    .left = 40,
    .right = 0
};

// Usable area calculation
UsableArea {
    .x = 40,                    // Left offset
    .y = 30,                    // Top offset
    .width = 1920 - 40,         // Screen width - left - right
    .height = 1080 - 30 - 25    // Screen height - top - bottom
};  // Result: 1880x1025 usable
```

## Files Created/Modified

### New Files
1. **`docs/STATUS_BARS.md`**
   - Comprehensive status bar documentation
   - Configuration examples
   - Widget types reference
   - Position options explained

2. **`config/status-bars-example.yaml`**
   - Complete example configurations
   - Multiple monitor setups
   - Different bar positions
   - Multi-bar examples

### Modified Files
1. **`include/config/ConfigParser.hpp`**
   - Added `StatusBarConfig` struct
   - Added `WidgetConfig` struct
   - Added `ContainerConfig` struct
   - Added `StatusBarsConfig` struct
   - Added `status_bars` vector to `MonitorConfig`
   - Added `ParseStatusBars()` method
   - Added `StatusBarsConfig::FindByName()` helper

2. **`src/config/ConfigParser.cpp`**
   - Implemented `ParseStatusBars()` (120+ lines)
   - Enhanced `ParseMonitorGroups()` to parse `status-bars` field
   - Implemented `StatusBarsConfig::FindByName()`
   - Added parsing calls in `Load()` and `LoadWithIncludes()`

3. **`ARCHITECTURE.md`**
   - Added Status Bars section
   - Explained reserved space integration
   - Provided configuration examples
   - Linked to detailed documentation

## Usage Examples

### Simple Setup
```yaml
status-bars:
  - name: main
    position: top
    height: 28
    left:
      widgets:
        - type: workspaces
    center:
      widgets:
        - type: clock

monitor-groups:
  - name: Default
    monitors:
      - display: eDP-1
        status-bar: main  # Single bar shorthand
```

### Multi-Monitor Different Bars
```yaml
status-bars:
  - name: laptop-detailed
    position: top
    # ...
  
  - name: monitor-minimal
    position: bottom
    # ...

monitor-groups:
  - name: Docked
    monitors:
      - display: eDP-1
        status-bars: [laptop-detailed]
      
      - display: HDMI-A-1
        status-bars: [monitor-minimal]
```

### Multiple Bars Per Monitor
```yaml
monitor-groups:
  - name: Workspace
    monitors:
      - display: eDP-1
        status-bars:
          - top-info-bar
          - bottom-system-bar
```

### No Status Bars
```yaml
monitor-groups:
  - name: Presentation
    monitors:
      - display: HDMI-A-1
        # Omit status-bars field = no bars
```

## Integration Points

### 1. LayerManager
- `SetReservedSpace()` - Bars reserve edge space
- `CalculateUsableArea()` - Returns area minus reserved space
- `TileViews()` - Uses reduced area for window layout

### 2. StatusBar Class (to be implemented)
```cpp
class StatusBar {
    StatusBar(const StatusBarConfig& config,
              LayerManager* layer_manager,
              int output_width,
              int output_height);
    
    void ReserveSpace();     // Register with LayerManager
    void CreateWidgets();     // Instantiate widget plugins
    void Render();           // Draw to scene
};
```

### 3. Output Management
```cpp
struct Output {
    wlr_output* wlr_output;
    LayerManager* layer_manager;
    vector<StatusBar*> status_bars;  // NEW: Multiple bars
    // ...
};
```

## Next Steps

### Immediate (To Complete Integration)
1. **Implement StatusBar::ReserveSpace()**
   - Calculate reserved pixels based on position and size
   - Call `layer_manager->SetReservedSpace()`
   - Handle multiple bars accumulating space

2. **Create StatusBars in OnNewOutput()**
   ```cpp
   void Server::OnNewOutput(wlr_output* wlr_output) {
       // ... existing output setup ...
       
       // Find monitor config
       auto* mon_config = FindMonitorConfig(wlr_output);
       
       // Create status bars
       for (const auto& bar_name : mon_config->status_bars) {
           auto* bar_config = Config().status_bars.FindByName(bar_name);
           if (bar_config) {
               auto* bar = new StatusBar(*bar_config, 
                                        output->layer_manager,
                                        wlr_output->width,
                                        wlr_output->height);
               output->status_bars.push_back(bar);
           }
       }
   }
   ```

3. **Update LayerManager::SetReservedSpace()**
   - Support accumulating multiple bars
   - Recalculate usable area when bars added/removed

### Future Enhancements
1. **Dynamic Status Bar Updates**
   - Hot-reload bar configuration
   - Add/remove bars without restart

2. **Widget Plugin System Integration**
   - Load widget types from plugins
   - Custom widget rendering

3. **Status Bar Styling**
   - Themes and color schemes
   - Per-widget styling
   - Transparency/blur effects

4. **Advanced Features**
   - Autohide bars
   - Hover to reveal
   - Bar animations
   - Workspace-specific bars

## Benefits

1. **Flexibility**: Any monitor can have any combination of bars
2. **Reusability**: Define once, use on multiple monitors
3. **Clarity**: Named bars make configuration readable
4. **Power**: Multiple bars per monitor for maximum customization
5. **Simplicity**: Clean YAML syntax
6. **Extensibility**: Widget plugin system for custom content

## Design Principles

1. **Configuration Over Code**: All bar setup in YAML
2. **Separation of Concerns**: Bar definition separate from monitor assignment
3. **Composability**: Widgets, containers, sections combine flexibly
4. **Convention Over Configuration**: Sensible defaults for common cases
5. **Progressive Enhancement**: Start simple, add complexity as needed

---

**Implementation Date**: December 19, 2025  
**Status**: Configuration parsing complete, StatusBar rendering pending
