# Wallpaper Engine Integration

This document describes the integration of linux-wallpaperengine as a wallpaper type in LeviathanDM.

## Overview

LeviathanDM now supports two types of wallpapers:
1. **Static Images** - Traditional image files (PNG, JPEG, BMP, WebP)
2. **Wallpaper Engine** - Animated wallpapers from Steam's Wallpaper Engine

## Architecture

### Components

1. **ConfigParser** (`include/config/ConfigParser.hpp`)
   - Added `WallpaperType` enum to distinguish between wallpaper types
   - Extended `WallpaperConfig` struct with `type` field
   - Updated YAML parser to read wallpaper type

2. **WallpaperManager** (`src/wayland/WallpaperManager.cpp`)
   - Handles both static images and Wallpaper Engine wallpapers
   - Manages wallpaper rotation for both types
   - Routes rendering based on wallpaper type

3. **WallpaperEngineRenderer** (`src/wayland/WallpaperEngineRenderer.cpp`)
   - Wrapper class for linux-wallpaperengine library
   - Manages WallpaperEngine application lifecycle
   - Renders animated wallpapers to wlroots scene graph

### Data Flow

```
Configuration (YAML)
        ↓
ConfigParser::ParseWallpapers()
        ↓
WallpaperConfig { type, paths, ... }
        ↓
WallpaperManager::InitializeWallpaper()
        ↓
   ┌────┴────┐
   ↓         ↓
Static    WallpaperEngine
Image     Renderer
   ↓         ↓
Scene     Scene
Node      Node
```

## Configuration

### YAML Syntax

```yaml
wallpapers:
  - name: my-wallpaper
    type: wallpaper_engine  # or "wallpaperengine", "we"
    wallpaper: /path/to/wallpaper/engine/project
    change_every_seconds: 0
```

### Wallpaper Types

- `static`, `image`, `static_image` - Static image wallpapers
- `wallpaper_engine`, `wallpaperengine`, `we` - Wallpaper Engine animated wallpapers

### Example Configuration

See `config/wallpaper-engine-example.yaml` for a complete example.

## Installation & Setup

### Prerequisites

1. **Wallpaper Engine** (via Steam)
   - Purchase and install Wallpaper Engine on Steam
   - Subscribe to wallpapers in the Steam Workshop

2. **linux-wallpaperengine Dependencies**
   ```bash
   sudo apt-get install libglew-dev libglfw3-dev libmpv-dev \
       libfftw3-dev libpulse-dev libsdl2-dev liblz4-dev \
       libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
   ```

3. **LeviathanDM with Wallpaper Engine support**
   ```bash
   cd /Projects/LeviathanDM
   mkdir -p build && cd build
   cmake ..
   make
   ```

### Finding Wallpaper Paths

Wallpaper Engine wallpapers are typically located at:
```
~/.steam/steam/steamapps/workshop/content/431960/<workshop_id>/
```

To find workshop IDs:
1. Browse https://steamcommunity.com/app/431960/workshop/
2. The ID is in the URL: `?id=XXXXXXXXXX`
3. Or check your local installation directory

## Current Status

### ✅ Implemented

- [x] Configuration parsing for wallpaper types
- [x] WallpaperManager routing based on type
- [x] WallpaperEngineRenderer skeleton
- [x] Static image wallpapers (fully working)
- [x] Build system integration

### ⚠️ In Progress

- [ ] **Full WallpaperEngine rendering integration**
  
  The linux-wallpaperengine library uses GLFW + OpenGL for rendering, while 
  LeviathanDM uses wlroots which has its own rendering pipeline. Deep integration
  is required to bridge these systems.

### 🔧 Technical Challenges

1. **OpenGL Context Management**
   - linux-wallpaperengine creates its own GLFW windows
   - wlroots uses its own renderer (OpenGL, Vulkan, or Pixman)
   - Need to share OpenGL context or render to shared textures

2. **Render Target Integration**
   - WallpaperEngine renders to GLFW framebuffers
   - wlroots expects wlr_buffer objects in scene graph
   - Need to copy/share framebuffer between systems

3. **Event Loop Integration**
   - WallpaperEngine has its own update loop
   - wlroots uses Wayland event loop
   - Need to coordinate rendering updates

## Development Roadmap

### Phase 1: Foundation (✅ Complete)
- Configuration system with wallpaper types
- WallpaperManager routing
- Basic WallpaperEngineRenderer structure

### Phase 2: Rendering Bridge (🚧 Future)
- Create shared OpenGL context between GLFW and wlroots
- Implement texture sharing or framebuffer copying
- Integrate WallpaperEngine render loop with Wayland event loop

### Phase 3: Feature Parity (📋 Planned)
- Support all WallpaperEngine wallpaper types (scene, video, web)
- Interactive wallpapers (mouse input)
- Audio support for wallpapers
- Performance optimizations

### Phase 4: Polish (📋 Planned)
- Smooth transitions between wallpapers
- Pause wallpapers when not visible
- Resource management and memory optimization

## Contributing

If you want to contribute to WallpaperEngine integration:

1. **Understand the systems**
   - Study `linux-wallpaperengine` source code
   - Study `wlroots` scene graph and renderer
   - Understand OpenGL context sharing

2. **Key files to modify**
   - `src/wayland/WallpaperEngineRenderer.cpp` - Main integration point
   - `subprojects/linux-wallpaperengine/` - Upstream library

3. **Testing**
   - Test with various wallpaper types (scene, video, web)
   - Test on different GPUs and drivers
   - Monitor performance and memory usage

## References

- [linux-wallpaperengine](https://github.com/Almamu/linux-wallpaperengine)
- [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
- [Wallpaper Engine Workshop](https://steamcommunity.com/app/431960/workshop/)
- [OpenGL Context Sharing](https://www.khronos.org/opengl/wiki/OpenGL_Context)

## FAQ

**Q: Why don't WallpaperEngine wallpapers render yet?**

A: The integration requires bridging two different rendering systems (GLFW/OpenGL 
and wlroots). This is complex and requires significant development work.

**Q: When will WallpaperEngine wallpapers be fully supported?**

A: This is a long-term goal. The infrastructure is in place, but the rendering 
integration is a substantial engineering effort.

**Q: Can I help with development?**

A: Yes! If you have experience with OpenGL, GLFW, and wlroots, contributions 
are welcome. See the Contributing section above.

**Q: Do static image wallpapers still work?**

A: Yes! Static image wallpapers work perfectly. Only WallpaperEngine animated 
wallpapers are not yet functional.

## License

This integration follows the same license as LeviathanDM. The linux-wallpaperengine
subproject retains its original license.
