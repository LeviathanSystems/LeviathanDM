---
title: "Release Notes"
weight: 2
---

# Release Notes

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
