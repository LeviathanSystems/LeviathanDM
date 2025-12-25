# LeviathanDM TODO List

## High Priority

### Borders
- [ ] **Bottom border not visible** - Only top, left, and right borders are showing
  - Current approach: Borders created as siblings of scene_tree using `scene_tree->node.parent`
  - Issue: Bottom border positioned at y=690 (workspace height) may be getting clipped by parent scene layer
  - Possible solutions to investigate:
    1. Create borders in a different scene layer that doesn't clip
    2. Use a container/wrapper scene tree approach (previously attempted, reverted due to complexity)
    3. Position borders absolutely in the scene graph
    4. Check if borders need to be recreated after client surface commit
  - Debug info: Logs show all 4 borders created with correct dimensions and positions
  - Status: Postponed for later investigation

### Window Management
- [ ] **SwapWithNext/SwapWithPrev** - Currently disabled during per-screen tag refactoring
  - Need to update to work with LayerManager's per-screen tags
  - Should swap clients within the focused screen's active tag

### Notifications
- [ ] **Visual rendering for notifications** - Protocol implemented, rendering not yet done
  - Full DBus protocol working (org.freedesktop.Notifications)
  - Notification storage and expiration handling complete
  - Need to implement:
    - Create Wayland layer surfaces for each notification
    - Render using Cairo (title, body, icon, actions)
    - Position in corner with vertical stacking
    - Handle clicks for actions and dismissal
    - Animate fade in/out transitions
  - Files: Create `include/ui/Notification.hpp` and `src/ui/Notification.cpp`
  - See `docs/NOTIFICATION_DAEMON.md` for details

## Medium Priority

### Layout System
- [ ] Add layout mode tracking to TilingLayout class
  - Track current layout mode (MasterStack, Monocle, Grid, Floating)
  - Expose via IPC for status bar indicators
  - Add keybindings to switch between layout modes

### Per-Screen Tags
- [ ] Add support for moving clients between screens
- [ ] Add keybindings to focus different screens
- [ ] Test with multi-monitor setup

### IPC Commands
- [ ] Implement missing IPC commands:
  - Layout switching commands
  - Client manipulation commands (move, resize, float/unfloat)
  - Screen focus commands

## Low Priority

### Code Quality
- [ ] Remove debug logging from production builds
  - Many LOG_DEBUG statements added during development
  - Consider adding debug build flag
- [ ] Add error handling for border creation failures
- [ ] Document the scene graph architecture in ARCHITECTURE.md

### Features
- [ ] Add border width/color configuration hot-reload
- [ ] Add per-tag layout configurations
- [ ] Add window rules (auto-float, auto-tag assignment)

## Completed ✓

- [x] Initialize LayerManager's layout_engine_
- [x] Fix AutoTile() automatic tiling
- [x] Fix client cleanup crash (remove from tag before delete)
- [x] Implement border system in View class
- [x] Add border color updates on focus changes
- [x] Fix border creation timing (moved to MoveResizeView)
- [x] Update config with border_color_focused and border_color_unfocused
- [x] Add per-screen tag system (AwesomeWM model)

## Notes

### Border Investigation Details
From logs (22:54:19.864):
```
CreateBorders: view=0x55f2b19ec6e0, width=1280, height=690, border_width=2
  Created top border: size=1284x2, pos=(-2,-2)
  Created right border: size=2x690, pos=(1280,0)
  Created bottom border: size=1284x2, pos=(-2,690)
  Created left border: size=2x690, pos=(-2,0)
```

All borders are created correctly. The issue is likely:
- Parent scene layer clips at workspace boundaries
- Bottom border at y=690 is at/beyond the clipping boundary
- Need to investigate wlroots scene graph clipping behavior

### Architecture Considerations
- Current approach: Borders as scene_rect children of parent layer
- Scene graph hierarchy: scene → layers → scene_tree (per window) + borders (siblings)
- Status bar reserves 30px at top, workspace is (0,30) to (1280,720)
