# Changelog - Version 0.0.4

**Release Date:** January 1, 2026

## Major Features

### Hierarchical Coordinate System & Widget Positioning Fixes
Fixed critical coordinate system issues in the widget rendering and event handling system.

#### What Changed
- **Before:** Widget positions accumulated when using containers, causing modals and popovers to display incorrectly
- **After:** Proper hierarchical coordinate system with `cairo_translate()` for rendering and coordinate transformation for events

#### Technical Details

##### 1. Coordinate Transformation Architecture
Implemented proper parent-child coordinate transformation:

```cpp
// Container rendering uses cairo_translate to establish local coordinate system
void Container::Render(cairo_t* cr) {
    cairo_save(cr);
    cairo_translate(cr, x_, y_);  // Establish local space
    
    for (auto& child : children_) {
        child->Render(cr);  // Children render in local coordinates
    }
    
    cairo_restore(cr);
}
```

##### 2. Event Coordinate Transformation
Matching transformation for click, hover, and scroll events:

```cpp
bool Container::HandleClick(int click_x, int click_y) {
    // Transform absolute screen coordinates to local space
    int local_x = click_x - x_;
    int local_y = click_y - y_;
    
    for (auto& child : children_) {
        if (child->HandleClick(local_x, local_y)) {
            return true;
        }
    }
    return false;
}
```

##### 3. Absolute Position Calculation
Added methods to Widget class to calculate absolute screen coordinates:

```cpp
// Walk up parent chain to get absolute position
int Widget::GetAbsoluteX() const {
    int abs_x = x_;
    Container* p = parent_;
    while (p) {
        abs_x += p->GetX();
        p = p->GetParent();
    }
    return abs_x;
}
```

These methods are used by popovers to position themselves on screen.

### Popover Architecture Refactoring
Moved popover rendering from StatusBar to LayerManager for cleaner separation of concerns.

#### What Changed
- **Before:** StatusBar handled its own popover rendering with duplicate buffer management
- **After:** LayerManager manages all Top layer UI (modals AND popovers) consistently

#### Benefits
- **Consistency:** Both modals and popovers now rendered by LayerManager
- **Code Reduction:** Removed 210+ lines from StatusBar
- **Maintainability:** Single source of truth for Top layer rendering
- **Scalability:** Easy to add more Top layer UI elements

#### Implementation

```cpp
// LayerManager now handles popover rendering
void LayerManager::RenderPopovers() {
    // Search all status bars for visible popovers
    for (auto* bar : status_bars_) {
        if (auto popover = bar->FindVisiblePopover()) {
            // Render to Top layer scene buffer
            RenderPopoverToBuffer(popover);
        }
    }
}
```

### Scrollable Modal Content Support
Implemented full scrolling support for modal dialogs with ScrollView widget.

#### New Features

##### 1. HandleScroll Event System
Added scroll event handling throughout the widget hierarchy:

```cpp
// Base Widget class
virtual bool HandleScroll(int x, int y, double delta_x, double delta_y) {
    return false;  // Override in subclasses
}

// Container forwards to children
bool Container::HandleScroll(int x, int y, double delta_x, double delta_y) {
    int local_x = x - x_;
    int local_y = y - y_;
    
    for (auto& child : children_) {
        if (child->HandleScroll(local_x, local_y, delta_x, delta_y)) {
            return true;
        }
    }
    return false;
}
```

##### 2. ScrollView Widget
Implemented scrollable container widget with:
- Vertical scrolling with mouse wheel
- Visible scrollbar with customizable appearance
- Clipping region for viewport
- Smooth scroll offset clamping

```cpp
auto scroll_view = std::make_shared<ScrollView>();
scroll_view->SetChild(content_widget);
scroll_view->SetScrollbarColor(0.6, 0.6, 0.6, 0.7);
scroll_view->ShowScrollbar(true);
```

##### 3. Wayland Input Integration
Integrated scroll events from Wayland pointer axis events:

```cpp
void InputManager::HandleCursorAxis(wl_listener* listener, void* data) {
    auto* event = static_cast<wlr_pointer_axis_event*>(data);
    
    // Forward to modals first
    if (server->CheckModalScroll(cursor_x, cursor_y, delta_x, delta_y)) {
        return;  // Modal handled it
    }
    
    // Otherwise pass to clients
    wlr_seat_pointer_notify_axis(...);
}
```

##### 4. Keybindings Modal Update
Updated keybindings help modal to use ScrollView:

```cpp
void KeybindingHelpModal::BuildWidgetContent() {
    auto main_vbox = std::make_shared<VBox>();
    // ... build content ...
    
    // Wrap in ScrollView
    auto scroll_view = std::make_shared<ScrollView>();
    scroll_view->SetChild(main_vbox);
    
    SetContent(scroll_view);
}
```

## Bug Fixes

### Widget Position Accumulation
- **Issue:** Modal showed wrong content on second open due to accumulated widget positions
- **Fix:** Implemented proper hierarchical coordinate system with `cairo_translate()`
- **Impact:** Modals, popovers, and all container-based layouts now render correctly

### Popover Positioning
- **Issue:** Popover appeared at (0,0) instead of near widget after coordinate transformation fix
- **Fix:** Added `GetAbsoluteX/Y()` methods to calculate screen position by walking parent chain
- **Impact:** Popovers now appear at correct location relative to trigger widget

### Click Event Handling
- **Issue:** Clicks not working on widgets with relative positions (coordinates mismatched)
- **Fix:** Container::HandleClick() now transforms absolute clicks to local space
- **Impact:** All nested widget interactions work correctly

### Missing Symbol in Plugins
- **Issue:** Battery widget plugin failed to load with undefined symbol `GetAbsoluteX`
- **Fix:** Added `src/ui/BaseWidget.cpp` to `libleviathan-ui` shared library
- **Impact:** Plugins can now use absolute positioning methods

## API Changes

### New Classes

#### `src/ui/BaseWidget.cpp`
New implementation file for Widget base class methods:
- `GetAbsoluteX()` - Calculate absolute X coordinate
- `GetAbsoluteY()` - Calculate absolute Y coordinate  
- `GetAbsolutePosition()` - Get both coordinates

### New Methods

#### Widget Class (`include/ui/BaseWidget.hpp`)
```cpp
// Scroll handling
virtual bool HandleScroll(int x, int y, double delta_x, double delta_y);

// Absolute position calculation
int GetAbsoluteX() const;
int GetAbsoluteY() const;
void GetAbsolutePosition(int& abs_x, int& abs_y) const;
```

#### Container Class (`include/ui/reusable-widgets/Container.hpp`)
```cpp
// Event handling with coordinate transformation
bool HandleScroll(int x, int y, double delta_x, double delta_y) override;
```

#### Modal Class (`include/ui/reusable-widgets/BaseModal.hpp`)
```cpp
// Scroll event forwarding
virtual bool HandleScroll(int x, int y, double delta_x, double delta_y);
```

#### LayerManager Class (`include/wayland/LayerManager.hpp`)
```cpp
// Popover rendering
void RenderPopovers();

// Modal scroll handling
bool HandleModalScroll(int x, int y, double delta_x, double delta_y);
bool HasVisibleModal() const;
```

#### Server Class (`include/wayland/Server.hpp`)
```cpp
// Modal scroll event checking
bool CheckModalScroll(int x, int y, double delta_x, double delta_y);
```

### Modified Methods

#### Container::Render()
Now uses `cairo_translate()` to establish local coordinate system for children.

#### Container::HandleClick() & HandleHover()
Now transform absolute screen coordinates to local space before forwarding to children.

## Files Added

- `src/ui/BaseWidget.cpp` - Widget base class implementation
- `src/ui/reusable-widgets/ScrollView.cpp` - Scrollable container widget (already existed, now integrated)

## Files Modified

### Core Widget System
- `include/ui/BaseWidget.hpp` - Added scroll handling and absolute positioning
- `include/ui/reusable-widgets/Container.hpp` - Added scroll event handling
- `src/ui/reusable-widgets/Container.cpp` - Coordinate transformation for all events
- `include/ui/reusable-widgets/ScrollView.hpp` - Fixed CalculateSize method signature
- `src/ui/reusable-widgets/ScrollView.cpp` - Fixed implementation and added M_PI include

### Modal System
- `include/ui/reusable-widgets/BaseModal.hpp` - Added HandleScroll method
- `src/ui/reusable-widgets/BaseModal.cpp` - Forward scroll to content widgets
- `src/ui/KeybindingHelpModal.cpp` - Wrapped content in ScrollView

### Popover System
- `include/wayland/LayerManager.hpp` - Added RenderPopovers and modal scroll methods
- `src/wayland/LayerManager.cpp` - Implemented popover rendering and modal scroll handling
- `src/ui/StatusBar.cpp` - Removed RenderPopoverToTopLayer (~210 lines)
- `include/ui/StatusBar.hpp` - Removed popover rendering members

### Input System
- `src/wayland/Input.cpp` - Implemented HandleCursorAxis for scroll events
- `include/wayland/Server.hpp` - Added CheckModalScroll method
- `src/wayland/Server.cpp` - Implemented modal scroll checking

### Build System
- `CMakeLists.txt` - Added BaseWidget.cpp and ScrollView.cpp to build

## Breaking Changes

None. All changes are internal improvements and bug fixes.

## Migration Guide

No migration needed. Existing code continues to work without changes.

### For Plugin Developers

If you're creating custom widgets that need absolute positioning:

```cpp
// Old way (didn't work correctly with nested containers)
int abs_x = x_;
int abs_y = y_;

// New way (correctly handles container hierarchy)
int abs_x = GetAbsoluteX();
int abs_y = GetAbsoluteY();
```

## Performance Impact

- Minimal performance impact from coordinate transformations
- ScrollView widget efficiently handles clipping and rendering
- Popover rendering now consolidated in LayerManager (slightly more efficient)

## Known Issues

None.

## Future Improvements

- Horizontal scrolling support in ScrollView
- Scroll event handling for popovers
- Momentum scrolling / kinetic scrolling
- Scroll animation/easing

## Credits

- Architecture improvements based on common UI framework patterns
- ScrollView implementation inspired by Flutter's SingleChildScrollView

---

**Full Diff:** [View on GitHub](https://github.com/LeviathanSystems/LeviathanDM/compare/v0.0.3...v0.0.4)
