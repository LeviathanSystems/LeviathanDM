---
title: "Release Notes"
weight: 2
---

# Release Notes

## Version 0.0.4 - Coordinate System & Scrolling

{{< version-banner version="v0.0.4" >}}

**Release Date:** January 1, 2026

### 🎉 Major Features

#### Hierarchical Coordinate System

Fixed critical coordinate system bugs affecting modal and popover positioning.

{{< hint info >}}
**What Changed**  
Implemented proper hierarchical coordinates with `cairo_translate()` for rendering and matching coordinate transformation for events. Widgets in nested containers now render and respond to clicks at the correct positions.
{{< /hint >}}

**Technical Details:**

```cpp
// Containers establish local coordinate systems
void Container::Render(cairo_t* cr) {
    cairo_save(cr);
    cairo_translate(cr, x_, y_);  // Children render in local space
    
    for (auto& child : children_) {
        child->Render(cr);
    }
    
    cairo_restore(cr);
}

// Event handlers transform coordinates to match
bool Container::HandleClick(int click_x, int click_y) {
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

#### Absolute Position Calculation

New methods to calculate screen coordinates for nested widgets:

```cpp
// Calculate absolute position by walking parent chain
int abs_x = widget->GetAbsoluteX();
int abs_y = widget->GetAbsoluteY();

// Used by popovers to position on screen
popover_->SetPosition(abs_x, abs_y + height_ + 2);
```

#### Scrollable Modal Content

Full scrolling support with the ScrollView widget for modals with lots of content.

**Features:**
- Mouse wheel scrolling
- Visible scrollbar with customizable appearance
- Smooth clipping and viewport management
- Integrated with keybindings help modal

**Example:**

```cpp
auto scroll_view = std::make_shared<ScrollView>();
scroll_view->SetChild(content_widget);
scroll_view->SetScrollbarColor(0.6, 0.6, 0.6, 0.7);
scroll_view->ShowScrollbar(true);

modal->SetContent(scroll_view);
```

#### Popover Architecture Refactoring

Moved popover rendering from StatusBar to LayerManager for consistency with modal rendering.

**Benefits:**
- Single source of truth for Top layer UI
- Removed 210+ lines of duplicate code from StatusBar
- Easier to maintain and extend

```cpp
// LayerManager now handles both modals and popovers
void LayerManager::RenderPopovers() {
    for (auto* bar : status_bars_) {
        if (auto popover = bar->FindVisiblePopover()) {
            RenderPopoverToBuffer(popover);
        }
    }
}
```

### 🐛 Bug Fixes

- **Fixed modal showing wrong content on second open**
  - Root cause: Widget positions accumulated in nested containers
  - Solution: Proper hierarchical coordinate system with cairo_translate

- **Fixed popover appearing at (0,0)**
  - Root cause: Using relative coordinates instead of absolute screen position
  - Solution: GetAbsoluteX/Y() methods walk parent chain to calculate screen coordinates

- **Fixed clicks not working on nested widgets**
  - Root cause: Event coordinates didn't match rendering coordinates
  - Solution: Container::HandleClick transforms absolute coords to local space

- **Fixed plugin loading error (undefined symbol)**
  - Root cause: BaseWidget methods not included in shared library
  - Solution: Created src/ui/BaseWidget.cpp and added to libleviathan-ui

### 🏗️ Architecture Changes

#### Event System

Added `HandleScroll` to widget event hierarchy:

```cpp
class Widget {
    virtual bool HandleScroll(int x, int y, double delta_x, double delta_y);
};

// Containers forward to children
// ScrollView implements scrolling behavior
// Modals forward to content widgets
```

#### Input Integration

Wayland pointer axis events now integrated with widget system:

```cpp
void InputManager::HandleCursorAxis(wl_listener* listener, void* data) {
    // Check modals first
    if (server->CheckModalScroll(cursor_x, cursor_y, delta_x, delta_y)) {
        return;  // Modal handled it
    }
    
    // Otherwise pass to clients
    wlr_seat_pointer_notify_axis(...);
}
```

### 📦 API Changes

**New Classes:**
- `src/ui/BaseWidget.cpp` - Implementation file for Widget methods

**New Methods:**
```cpp
// Widget class
virtual bool HandleScroll(int x, int y, double delta_x, double delta_y);
int GetAbsoluteX() const;
int GetAbsoluteY() const;
void GetAbsolutePosition(int& abs_x, int& abs_y) const;

// Container class  
bool HandleScroll(int x, int y, double delta_x, double delta_y) override;

// Modal class
virtual bool HandleScroll(int x, int y, double delta_x, double delta_y);

// LayerManager class
void RenderPopovers();
bool HandleModalScroll(int x, int y, double delta_x, double delta_y);
bool HasVisibleModal() const;

// Server class
bool CheckModalScroll(int x, int y, double delta_x, double delta_y);
```

**Modified Methods:**
- `Container::Render()` - Now uses cairo_translate for local coordinates
- `Container::HandleClick()` - Transforms coords to local space
- `Container::HandleHover()` - Transforms coords to local space

### 📝 Documentation

- Updated [Widget System Guide]({{< ref "/docs/development/widget-system.md" >}}) with coordinate system details
- New scrolling documentation in widget guides
- Migration notes for absolute positioning

### ⚠️ Breaking Changes

None. All changes are internal improvements and bug fixes. Existing code continues to work without modifications.

---

## Version 0.0.3 - Widget-Based Popovers

{{< version-banner version="v0.0.3" >}}

**Release Date:** December 30, 2025

### 🎉 Major Features

#### Widget-Based Popover System

Complete refactoring of the popover system from manual Cairo drawing to a composable widget architecture.

{{< hint info >}}
**What Changed**  
Popovers now accept widget hierarchies (VBox, HBox, Label) instead of requiring manual Cairo drawing code. This makes it much easier to create complex, flexible popover layouts.
{{< /hint >}}

**Example:**

```cpp
// Build popover with widgets
auto vbox = std::make_shared<VBox>();
vbox->SetSpacing(4);

auto row = std::make_shared<HBox>();
row->AddChild(std::make_shared<Label>("󰁹"));
row->AddChild(std::make_shared<Label>("Battery"));
row->AddChild(std::make_shared<Label>("95%"));

vbox->AddChild(row);
popover_->SetContent(vbox);
```

See the [Widget System Guide]({{< ref "/docs/development/widget-system.md" >}}) for complete documentation.

### 🏗️ Architecture Changes

#### IPopoverProvider Interface

Introduced a new interface-based pattern for widgets with popovers:

```cpp
class MyWidget : public PeriodicWidget, public IPopoverProvider {
    std::shared_ptr<Popover> GetPopover() const override;
    bool HasPopover() const override;
};
```

This replaces the inheritance-based approach where popovers were built into `BaseWidget`.

#### Dependency Cleanup

- Broke circular dependency between `BaseWidget` and `Popover`
- Moved `Popover` to `ui/reusable-widgets/` for better organization
- Simplified `BaseWidget` by removing popover-specific logic

### 🐛 Bug Fixes

- **Fixed popover content disappearing on subsequent opens**
  - Root cause: Nested container widgets weren't recalculating child positions
  - Solution: Implemented recursive position recalculation for entire widget tree
  
- **Fixed widgets rendering off-screen in popovers**
  - HBox children (Labels) weren't updating when parent VBox moved
  - Now recursively updates all containers in the widget hierarchy

### 📦 Updated Components

**Modified Files:**
- `include/ui/reusable-widgets/Popover.hpp` - Standalone reusable widget
- `src/ui/Popover.cpp` - Recursive container handling
- `include/ui/IPopoverProvider.hpp` - New interface
- `include/ui/BaseWidget.hpp` - Simplified
- `src/ui/StatusBar.cpp` - Uses IPopoverProvider
- `plugins/battery-widget/` - Updated to widget-based approach

**Build System:**
- Added `src/ui/Popover.cpp` to CMakeLists.txt

### 📝 Documentation

- New comprehensive [Widget System & Popovers]({{< ref "/docs/development/widget-system.md" >}}) guide
- Code examples for building widget hierarchies
- Migration guide from old PopoverItem system

### ⚠️ Breaking Changes

{{< hint warning >}}
**For Plugin Developers**

- `BaseWidget` no longer has built-in popover support
- Widgets that need popovers must implement `IPopoverProvider` interface
- Direct popover access through `BaseWidget::popover_` is removed

See the [migration guide]({{< ref "/docs/development/widget-system.md#migration-from-popoveritem" >}}) for details.
{{< /hint >}}

### 🔮 Deprecated

{{< hint danger >}}
**PopoverItem System Deprecated**

The old `PopoverItem` struct-based approach is deprecated and will be removed in a future version. Please migrate to the widget-based approach.

```cpp
// ❌ Old (deprecated)
popover_->AddItem("icon", "text", "detail");

// ✅ New (preferred)
auto row = std::make_shared<HBox>();
row->AddChild(std::make_shared<Label>("icon"));
popover_->SetContent(row);
```
{{< /hint >}}

### 📊 Technical Details

#### Recursive Position Recalculation

The fix for nested container positioning uses a recursive algorithm:

```cpp
static void RecalculateContainerTree(Widget* widget) {
    auto* container = dynamic_cast<Container*>(widget);
    if (!container) return;
    
    container->CalculateSize(10000, 10000);
    
    const auto& children = container->GetChildren();
    for (const auto& child : children) {
        RecalculateContainerTree(child.get());
    }
}
```

This ensures that when a popover is temporarily moved to (0,0) for rendering to its own Cairo surface, all widgets in the tree (VBox → HBox → Labels) update their positions correctly.

### 🎯 Benefits

**For Users:**
- More visually consistent and appealing popovers
- Better text rendering with proper font support
- Reliable popover display on every open

**For Developers:**
- Composable widget system is easier to understand
- Flexible layout without manual Cairo drawing
- Type-safe interface-based design
- Better code organization and maintainability

### 📚 Resources

- [Widget System Guide]({{< ref "/docs/development/widget-system.md" >}})
- [Plugin Development]({{< ref "/docs/development/plugins.md" >}})
- [CHANGELOG-0.0.3.md](https://github.com/LeviathanSystems/LeviathanDM/blob/master/CHANGELOG-0.0.3.md)

---

## Version 0.0.2

See [previous releases](https://github.com/LeviathanSystems/LeviathanDM/releases) on GitHub.
