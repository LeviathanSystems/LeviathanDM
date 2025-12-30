# Changelog - Version 0.0.3

**Release Date:** December 30, 2025

## Major Features

### Widget-Based Popover System
Completely refactored the popover system to use composable widgets instead of manual Cairo drawing.

#### What Changed
- **Before:** Popovers required manual Cairo drawing with `PopoverItem` structs
- **After:** Popovers accept widget hierarchies (VBox, HBox, Label, etc.) for flexible layouts

#### New Architecture

##### 1. Popover as Reusable Component
- Moved `Popover` from `BaseWidget` to `ui/reusable-widgets/Popover.hpp`
- Broke circular dependency between `BaseWidget` and `Popover`
- Popover now accepts `std::shared_ptr<Widget>` as content

##### 2. IPopoverProvider Interface
```cpp
class IPopoverProvider {
public:
    virtual ~IPopoverProvider() = default;
    virtual std::shared_ptr<Popover> GetPopover() const = 0;
    virtual bool HasPopover() const = 0;
};
```

Widgets that want popover functionality implement this interface instead of inheriting popover logic from `BaseWidget`.

##### 3. Composable Widget Hierarchy
Popovers can now be built using widget composition:

```cpp
// Create a VBox container
auto vbox = std::make_shared<VBox>();
vbox->SetSpacing(4);

// Create an HBox row with labels
auto row = std::make_shared<HBox>();
row->SetSpacing(8);
row->AddChild(std::make_shared<Label>("Icon"));
row->AddChild(std::make_shared<Label>("Text"));
row->AddChild(std::make_shared<Label>("Value"));

// Add row to VBox
vbox->AddChild(row);

// Set as popover content
popover_->SetContent(vbox);
```

#### Technical Implementation Details

##### Widget Tree Rendering
The popover rendering system handles complex nested widget trees:

**Widget Hierarchy:**
```
Popover
└─ VBox (content_widget_)
   ├─ HBox (row 1)
   │  ├─ Label (icon)
   │  ├─ Label (text)
   │  └─ Label (value)
   ├─ HBox (row 2 - separator)
   │  └─ Label (line)
   └─ HBox (row 3)
      ├─ Label (icon)
      ├─ Label (text)
      └─ Label (value)
```

##### Recursive Position Recalculation
A critical fix was implemented to handle nested containers:

**Problem:** When StatusBar renders a popover to a separate Cairo surface, it temporarily moves the popover to position (0,0). The top-level container (VBox) would update its child positions, but nested containers (HBox) wouldn't update *their* children (Labels), causing widgets to render off-screen.

**Solution:** Implemented `RecalculateContainerTree()` that recursively recalculates positions for all nested containers:

```cpp
static void RecalculateContainerTree(Widget* widget) {
    auto* container = dynamic_cast<Container*>(widget);
    if (!container) return;
    
    // Recalculate this container's size and child positions
    container->CalculateSize(10000, 10000);
    
    // Recursively recalculate all child containers
    const auto& children = container->GetChildren();
    for (const auto& child : children) {
        RecalculateContainerTree(child.get());
    }
}
```

This ensures the entire widget tree (VBox → HBox → Labels) is correctly positioned relative to the popover's current render position.

##### Cairo Surface Management
- Each popover renders to its own Cairo surface for proper layering
- Surface is created once and reused across renders (recreated only when size changes)
- Surface is cleared with `CAIRO_OPERATOR_CLEAR` before each render
- Popover position is temporarily adjusted to (0,0) for rendering to its own surface
- Widget tree is recursively recalculated to ensure correct relative positioning

#### Files Modified

**Core Changes:**
- `include/ui/reusable-widgets/Popover.hpp` - Popover as standalone reusable widget
- `src/ui/Popover.cpp` - Implementation with recursive container handling
- `include/ui/IPopoverProvider.hpp` - New interface for popover functionality
- `include/ui/BaseWidget.hpp` - Simplified, removed popover dependencies
- `src/ui/StatusBar.cpp` - Uses `IPopoverProvider` interface

**Plugin Updates:**
- `plugins/battery-widget/BatteryWidget.cpp` - Implements `IPopoverProvider`
- `plugins/battery-widget/BatteryWidget.hpp` - Updated to use widget-based popover

**Build System:**
- `CMakeLists.txt` - Added `src/ui/Popover.cpp` to build

**Cleanup:**
- Removed duplicate `include/ui/Popover.hpp` (old version)

## Benefits

### For Users
- More visually appealing and consistent popovers
- Better text rendering with proper font support
- Content properly displays on every popover open

### For Developers
- **Composability:** Build complex layouts using VBox, HBox, Label widgets
- **Flexibility:** Easy to add buttons, icons, or custom widgets to popovers
- **Maintainability:** Widget-based approach is easier to understand and modify
- **Reusability:** Popover is now a standalone component that any widget can use
- **Type Safety:** Interface-based design (IPopoverProvider) instead of inheritance

## Migration Guide

### For Existing Plugins

**Old Approach (PopoverItem):**
```cpp
popover_->AddItem("Icon", "Main text", "Detail");
popover_->AddSeparator();
popover_->CalculateSize();
```

**New Approach (Widget-Based):**
```cpp
auto vbox = std::make_shared<VBox>();
vbox->SetSpacing(4);

auto row = std::make_shared<HBox>();
row->SetSpacing(8);
row->AddChild(std::make_shared<Label>("Icon"));
row->AddChild(std::make_shared<Label>("Main text"));
row->AddChild(std::make_shared<Label>("Detail"));

vbox->AddChild(row);
popover_->SetContent(vbox);
```

### Implementing Popovers in New Widgets

1. **Include the interface:**
```cpp
#include "ui/IPopoverProvider.hpp"
#include "ui/reusable-widgets/Popover.hpp"
```

2. **Implement IPopoverProvider:**
```cpp
class MyWidget : public PeriodicWidget, public IPopoverProvider {
public:
    std::shared_ptr<Popover> GetPopover() const override { 
        return popover_; 
    }
    
    bool HasPopover() const override { 
        return popover_ != nullptr; 
    }

private:
    std::shared_ptr<Popover> popover_;
    void UpdatePopover(); // Your custom method to build popover content
};
```

3. **Build popover content using widgets:**
```cpp
void MyWidget::UpdatePopover() {
    if (!popover_) {
        popover_ = std::make_shared<Popover>();
    }
    
    // Clear old content
    popover_->ClearContent();
    
    // Build new widget hierarchy
    auto vbox = std::make_shared<VBox>();
    // ... add children ...
    
    // Set as popover content
    popover_->SetContent(vbox);
}
```

## Bug Fixes
- Fixed popover content disappearing on subsequent opens
- Fixed nested container widgets not updating positions correctly
- Resolved circular dependency between BaseWidget and Popover

## Known Issues
- Legacy `PopoverItem` system is still supported but may be deprecated in future versions
- Popover animations not yet implemented

## Breaking Changes
- `BaseWidget` no longer has built-in popover support
- Widgets that want popovers must implement `IPopoverProvider` interface
- Direct popover access through `BaseWidget::popover_` is no longer available

## Technical Notes

### Performance Considerations
- Widget trees are recalculated on every popover render
- For large widget hierarchies, consider caching calculated sizes
- Cairo surface is reused between renders to minimize memory allocation

### Future Improvements
- Add popover animations (fade in/out, slide)
- Implement popover positioning strategies (auto-position to avoid screen edges)
- Add support for interactive popover content (buttons, sliders, etc.)
- Consider caching widget tree calculations for better performance

---

**Contributors:** LeviathanSystems
**Tested On:** Wayland (cage compositor)
**Requires:** Cairo, wlroots 0.19
