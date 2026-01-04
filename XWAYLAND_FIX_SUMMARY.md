# XWayland Chrome Crash Fix Summary

## Issues Fixed ✅

### 1. **Compositor No Longer Crashes**
- **Problem**: When Chrome crashed, it caused a broken pipe that crashed the entire compositor
- **Solution**: Added error handling in the event loop to gracefully handle `EPIPE` errors
- **Code**: `src/wayland/Server.cpp` - Event loop now catches and logs errors without crashing

### 2. **Proper XWayland Surface Validation**
- **Problem**: Trying to create views for XWayland surfaces before they're ready
- **Solution**: Added validation to check if `wl_surface` exists before creating view
- **Code**: `OnNewXwaylandSurface()` now returns early for incomplete surfaces

### 3. **Better Focus Handling**
- **Problem**: Attempting to focus destroyed surfaces
- **Solution**: Added surface validation in `FocusView()` and clear keyboard focus in `RemoveView()`
- **Code**: Validates surface before focusing and calls `wlr_seat_keyboard_clear_focus()` on cleanup

### 4. **Proper XWayland Surface Closing**
- **Problem**: Using wrong close function for XWayland surfaces
- **Solution**: Use `wlr_xwayland_surface_close()` for X11, `wlr_xdg_toplevel_send_close()` for Wayland
- **Code**: `CloseView()` now checks surface type and uses appropriate close function

## Current Issue ❌

### Chrome Crashes Before Window Appears

**Symptoms**:
```
[glfw error 65544]: Wayland: fatal display error: Broken pipe
(EE) failed to read Wayland events: Broken pipe
```

**Analysis**:
- Chrome launches successfully
- XWayland server starts (`:2`)
- Chrome creates multiple X11 surfaces (normal behavior)
- **But Chrome's GPU process crashes** before the main window maps
- The compositor now handles this crash gracefully without crashing itself

**Logs show**:
```
[13:27:22.300] [info] New Xwayland surface: title='(null)'
[13:27:22.300] [warn] XWayland surface has no wl_surface, skipping  <-- This is NORMAL
```

These warnings are **expected** - X11 apps create multiple helper surfaces during startup.

## Why Chrome is Crashing

Chrome's error messages indicate GPU/rendering issues:
```
ERROR:broker_posix.cc(44)] Invalid node channel message
ERROR:gpu_channel.cc(502)] Buffer Handle is null
```

### Possible Causes:

1. **GPU Acceleration Issues**
   - Chrome's GPU process can't initialize properly under XWayland
   - Solution: Try `--disable-gpu` flag

2. **Shared Memory Issues**
   - DRI/Mesa driver compatibility
   - Buffer sharing between Chrome and XWayland failing

3. **Chrome Sandbox Conflicts**
   - Chrome's security sandbox may conflict with nested Wayland/XWayland
   - Solution: Try `--no-sandbox` (only for testing!)

## Recommended Next Steps

### 1. Test with Simpler X11 Apps
```bash
DISPLAY=:2 xterm      # Should work
DISPLAY=:2 xclock     # Should work  
DISPLAY=:2 firefox    # More complex, good test
```

### 2. Try Chrome with Compatibility Flags
```bash
DISPLAY=:2 google-chrome-stable \
  --disable-gpu \
  --disable-software-rasterizer \
  --no-sandbox
```

### 3. Use Native Wayland Chrome (Recommended!)
```bash
# Chrome has native Wayland support
google-chrome-stable \
  --enable-features=UseOzonePlatform \
  --ozone-platform=wayland
```

### 4. Check wlroots Debug Logs
The wlroots messages `[xwayland/xwm.c:1105] unhandled X11 property` are **informational only** - not errors.

## Testing Checklist

- [ ] Test xterm: `DISPLAY=:2 xterm`
- [ ] Test xclock: `DISPLAY=:2 xclock`
- [ ] Test Firefox: `DISPLAY=:2 firefox`
- [ ] Test Chrome with --disable-gpu
- [ ] Test Chrome native Wayland mode
- [ ] Monitor `leviathan.log` for actual errors

## Success Criteria

✅ **Compositor no longer crashes** - ACHIEVED!
⏳ **Chrome works via XWayland** - Still investigating
✅ **Graceful error handling** - ACHIEVED!

## Files Modified

1. `src/wayland/Server.cpp`
   - Added errno handling for event loop
   - Improved XWayland surface validation
   - Better keyboard focus management
   - Fixed CloseView() for both surface types

2. `include/wayland/XwaylandCompat.hpp`
   - Added `wlr_xwayland_surface_close()` declaration

---

**Last Updated**: January 4, 2026
**Status**: Compositor stability fixed; Chrome GPU crash under investigation
