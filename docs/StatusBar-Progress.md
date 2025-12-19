# Status Bar Implementation Progress

## ✅ Completed

### 1. Widget System Architecture
- **Widget Base Class**: Polymorphic base with `Render()`, `CalculateSize()`, and property management
- **Built-in Widgets**: Label, HBox, VBox implemented with Cairo rendering
- **Configuration System**: YAML-based widget configuration with properties (text, font-size, color, spacing)

### 2. Plugin System
- **WidgetPluginManager**: Dynamic plugin loading from multiple directories
- **Plugin Interface**: Clean API with `Create()`, `Destroy()`, `GetMetadata()`, `GetVersion()`
- **Example Plugin**: ClockWidget (updates every second with current time)
- **Plugin Discovery**: Auto-scan `~/.config/leviathan/plugins/`, `/usr/local/lib/leviathan/plugins/`, `/usr/lib/leviathan/plugins/`

### 3. Cairo Rendering
- **Render Pipeline**: Widget tree → Cairo surface (ARGB32) → GPU texture
- **Three-Section Layout**: Left (left-aligned), Center (centered), Right (right-aligned)
- **Debug Output**: `/tmp/statusbar_debug.png` proves rendering works perfectly (1280x30)
- **GPU Upload**: `wlr_texture_from_pixels()` successfully creates texture from Cairo buffer

### 4. Proof of Concept Display
- **Pixel Rendering**: Using `wlr_scene_rect` nodes to display each pixel
- **Current Status**: ~112 pixel rects rendered (sample rate: 2)
- **Verified Working**: ClockWidget displays on screen, proves entire pipeline functional

## 🔧 Current Implementation

### StatusBar.cpp - Key Methods
```cpp
RenderToBuffer()     // Cairo rendering: widgets → ARGB32 buffer
UploadToTexture()    // wlr_texture_from_pixels + scene_rect pixel display
CreateWidgets()      // Factory pattern for built-in + plugin widgets
CreateSceneNodes()   // wlr_scene_rect background + widget layer
```

### Working Features
✅ Widget configuration parsing (YAML)
✅ Plugin loading and instantiation
✅ Cairo rendering to off-screen buffer
✅ Texture creation from pixel data
✅ On-screen display (via scene_rect hack)
✅ ClockWidget plugin loads and renders
✅ Background color/opacity
✅ Position configuration (Top/Bottom/Left/Right)

## ⚠️ Known Limitations

### Inefficient Rendering
**Problem**: Using `wlr_scene_rect` to render individual pixels
- Creates ~112 scene nodes for a 1280x30 bar (sample rate 2)
- Full resolution would need 38,400 nodes (completely impractical)
- No damage tracking or updates

**Why**: Cannot create `wlr_buffer` without internal wlroots APIs
- `wlr_buffer_init()` not exported by wlroots library
- `wlr_scene_buffer_set_buffer()` requires `wlr_buffer`, not `wlr_texture`
- Custom buffer implementations need unexported `wlr_buffer_impl` callbacks

## 🎯 Next Steps

### Option A: Proper SHM Buffer (Recommended for Compositor)
**Approach**: Create a shared memory buffer that wlroots can import
- Use `memfd_create()` + `mmap()` for SHM
- Implement `wlr_buffer` interface with `WLR_BUFFER_CAP_DATA_PTR`
- Let wlroots call `begin_data_ptr_access()` to get Cairo pixels
- Use `wlr_scene_buffer_set_buffer_with_damage()` for updates

**Challenge**: `wlr_buffer_init()` is internal, need alternative initialization

### Option B: External Status Bar Client
**Approach**: Separate Wayland client using layer-shell protocol
- Build `leviathanbar` as standalone executable
- Connect via Wayland protocol (like waybar, yambar, eww)
- Use `wlr-layer-shell-unstable-v1` for proper positioning
- Direct SHM buffer submission (no compositor-side hacks)

**Advantages**:
- Clean separation of concerns
- Can crash/restart independently
- Standard Wayland protocols
- Easier to develop and debug

### Option C: Compositor Render Hook
**Approach**: Render texture directly during output frame callback
- Hook into `wlr_output` frame event
- Use `wlr_render_texture_with_matrix()` during render pass
- Direct texture → framebuffer rendering

**Challenges**:
- More complex integration
- Tighter coupling to compositor internals
- Need to manage render timing manually

## 📊 Performance Metrics

### Current (Proof of Concept)
- **Pixels Rendered**: 112 (2x2 rects, sample rate 2)
- **Full Resolution**: Would need 38,400 nodes ❌
- **Update Method**: Full re-render + recreation
- **FPS Impact**: Unknown (likely significant with full resolution)

### Target (Proper Implementation)
- **Pixels**: Single wlr_buffer (38,400 pixels, one scene node)
- **Update Method**: Damage-tracked partial updates
- **FPS Impact**: Minimal (standard scene graph usage)

## 🧪 Testing Results

### Verified Working
```
[19:03:29.190] [info] Saved status bar rendering to /tmp/statusbar_debug.png for debugging
[19:03:29.190] [debug] Rendered status bar 'laptop-bar' with 2 total widgets to Cairo buffer
[19:03:29.190] [info] Created/updated texture 1280x30 for status bar 'laptop-bar'
[19:03:29.190] [info] Rendered 112 pixel rects for status bar (sample rate: 2, max: 5000)
```

### Debug PNG Analysis
- **Size**: 1280x30 pixels
- **Format**: RGBA non-interlaced
- **Content**: "LeviathanDM" (left) + Clock time (center)
- **Rendering**: Perfect, all widgets visible

## 📝 Code Organization

```
src/ui/
  ├── StatusBar.cpp        # Main status bar implementation
  ├── Widget.cpp           # Base widget class + built-in widgets
  └── WidgetPluginManager.cpp  # Plugin loading/management

include/ui/
  ├── StatusBar.hpp        # StatusBar class declaration
  ├── Widget.hpp           # Widget hierarchy
  └── WidgetPluginManager.hpp  # Plugin manager interface

plugins/clock-widget/
  ├── CMakeLists.txt       # Plugin build system
  └── clock_widget.cpp     # Example plugin implementation
```

## 🚀 Recommendation

**Proceed with Option B (External Status Bar Client)** because:

1. **Clean Architecture**: Follows industry standard (waybar, yambar, eww)
2. **No Hacks**: Uses proper Wayland protocols
3. **Reusable**: Can work with any wlroots compositor
4. **Development**: Easier to test and debug
5. **Crash Isolation**: Won't bring down compositor
6. **Future**: Can be distributed separately

Already started in `statusbar/` directory with:
- README.md (architecture documentation)
- CMakeLists.txt (build system)
- Protocol: wlr-layer-shell-unstable-v1

**Alternative**: If keeping internal is critical, investigate wlroots source for SHM buffer creation patterns or ask wlroots developers for compositor-side buffer best practices.
