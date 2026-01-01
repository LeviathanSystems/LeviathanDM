---
title: "Widget System & Popovers"
weight: 3
version: "v0.0.3"
description: "Learn how to build composable UI widgets and popovers in LeviathanDM"
---

# Widget System & Popovers

{{< version-warning type="new" version="v0.0.3" >}}
The widget-based popover system was introduced in v0.0.3, replacing the manual Cairo drawing approach.
{{< /version-warning >}}

LeviathanDM provides a composable widget system for building flexible UI components, particularly for status bar widgets and popovers.

## Overview

The widget system uses a hierarchical approach where complex layouts are built by composing simple widgets together:

- **Container Widgets**: `VBox` (vertical), `HBox` (horizontal)
- **Content Widgets**: `Label`, `Button`
- **Special Widgets**: `Popover`

## Widget Hierarchy

Widgets follow a parent-child relationship:

```
Popover
└─ VBox (content container)
   ├─ HBox (row 1)
   │  ├─ Label (icon)
   │  ├─ Label (text)
   │  └─ Label (value)
   ├─ HBox (row 2)
   │  └─ Label (separator)
   └─ HBox (row 3)
      ├─ Label (icon)
      ├─ Label (text)
      └─ Label (value)
```

## Building a Popover

### Step 1: Implement IPopoverProvider

Your widget must implement the `IPopoverProvider` interface:

```cpp
#include "ui/IPopoverProvider.hpp"
#include "ui/reusable-widgets/Popover.hpp"

class MyWidget : public PeriodicWidget, public IPopoverProvider {
public:
    // IPopoverProvider interface
    std::shared_ptr<Popover> GetPopover() const override { 
        return popover_; 
    }
    
    bool HasPopover() const override { 
        return popover_ != nullptr; 
    }

private:
    std::shared_ptr<Popover> popover_;
    void UpdatePopover(); // Your method to build content
};
```

### Step 2: Create Widget Hierarchy

Build your popover content using composable widgets:

```cpp
void MyWidget::UpdatePopover() {
    if (!popover_) {
        popover_ = std::make_shared<Popover>();
    }
    
    // Clear old content
    popover_->ClearContent();
    
    // Create vertical container
    auto vbox = std::make_shared<VBox>();
    vbox->SetSpacing(4); // 4px spacing between rows
    
    // Create first row
    auto row1 = std::make_shared<HBox>();
    row1->SetSpacing(8);
    
    auto icon = std::make_shared<Label>("󰁹");
    icon->SetFontSize(16);
    
    auto name = std::make_shared<Label>("Battery");
    name->SetFontSize(12);
    
    auto value = std::make_shared<Label>("95%");
    value->SetFontSize(10);
    value->SetTextColor(0.5, 0.8, 0.5, 1.0); // Green
    
    row1->AddChild(icon);
    row1->AddChild(name);
    row1->AddChild(value);
    
    // Add row to VBox
    vbox->AddChild(row1);
    
    // Create separator
    auto separator = std::make_shared<HBox>();
    auto line = std::make_shared<Label>("────────────────");
    line->SetFontSize(8);
    line->SetTextColor(0.3, 0.3, 0.3, 1.0);
    separator->AddChild(line);
    vbox->AddChild(separator);
    
    // Set content
    popover_->SetContent(vbox);
}
```

## Widget Reference

### Container: VBox

Vertical container that stacks children vertically.

```cpp
auto vbox = std::make_shared<VBox>();
vbox->SetSpacing(4);           // Space between children
vbox->SetAlign(Align::Start);   // Top-align children
vbox->AddChild(widget);
```

**Alignment Options:**
- `Align::Start` - Top-aligned
- `Align::Center` - Vertically centered
- `Align::End` - Bottom-aligned

### Container: HBox

Horizontal container that arranges children in a row.

```cpp
auto hbox = std::make_shared<HBox>();
hbox->SetSpacing(8);           // Space between children
hbox->SetAlign(Align::Center);  // Center-align children
hbox->AddChild(widget);
```

**Alignment Options:**
- `Align::Start` - Left-aligned
- `Align::Center` - Horizontally centered
- `Align::End` - Right-aligned
- `Align::Apart` - First, middle, last distribution (for status bars)

### Widget: Label

Displays text with customizable styling.

```cpp
auto label = std::make_shared<Label>("Hello World");
label->SetFontSize(12);
label->SetFontFamily("JetBrainsMono Nerd Font");
label->SetTextColor(1.0, 1.0, 1.0, 1.0);      // RGBA (white)
label->SetBackgroundColor(0.0, 0.0, 0.0, 0.0); // Transparent
```

**Methods:**
- `SetText(string)` - Update label text (thread-safe)
- `GetText()` - Get current text
- `SetFontSize(int)` - Set font size in points
- `SetFontFamily(string)` - Set font family name
- `SetTextColor(r, g, b, a)` - Set text color (values 0.0-1.0)
- `SetBackgroundColor(r, g, b, a)` - Set background color

### Widget: Popover

Container for popup content attached to widgets.

```cpp
auto popover = std::make_shared<Popover>();
popover->SetContent(widget);    // Set widget hierarchy as content
popover->ClearContent();        // Clear existing content
popover->Show();                // Show popover
popover->Hide();                // Hide popover
```

## Example: Battery Widget Popover

Here's the real implementation from the battery widget:

```cpp
void BatteryWidget::UpdatePopover() {
    if (!popover_) {
        popover_ = std::make_shared<Popover>();
    }
    
    popover_->ClearContent();
    
    auto vbox = std::make_shared<VBox>();
    vbox->SetSpacing(4);
    
    // Add main battery info
    auto main_row = std::make_shared<HBox>();
    main_row->SetSpacing(8);
    
    auto battery_icon = std::make_shared<Label>(GetBatteryIcon());
    battery_icon->SetFontSize(16);
    
    auto battery_name = std::make_shared<Label>("Main Battery");
    battery_name->SetFontSize(12);
    
    auto battery_percent = std::make_shared<Label>(
        std::to_string(battery_percentage_) + "%"
    );
    battery_percent->SetFontSize(10);
    
    main_row->AddChild(battery_icon);
    main_row->AddChild(battery_name);
    main_row->AddChild(battery_percent);
    vbox->AddChild(main_row);
    
    // Add separator if there are devices
    if (!devices_.empty()) {
        auto sep_row = std::make_shared<HBox>();
        auto separator = std::make_shared<Label>("────────────────");
        separator->SetFontSize(8);
        separator->SetTextColor(0.4, 0.4, 0.4, 1.0);
        sep_row->AddChild(separator);
        vbox->AddChild(sep_row);
    }
    
    // Add each device
    for (const auto& device : devices_) {
        auto device_row = std::make_shared<HBox>();
        device_row->SetSpacing(8);
        
        auto dev_icon = std::make_shared<Label>(GetDeviceIcon(device.type));
        dev_icon->SetFontSize(16);
        
        auto dev_name = std::make_shared<Label>(device.name);
        dev_name->SetFontSize(10);
        
        auto dev_percent = std::make_shared<Label>(
            std::to_string(device.percentage) + "%"
        );
        dev_percent->SetFontSize(10);
        
        device_row->AddChild(dev_icon);
        device_row->AddChild(dev_name);
        device_row->AddChild(dev_percent);
        vbox->AddChild(device_row);
    }
    
    popover_->SetContent(vbox);
}
```

## Technical Details

### Rendering Pipeline

1. **Widget Tree Creation**: Build hierarchy of VBox/HBox/Label widgets
2. **Size Calculation**: Each widget calculates its preferred size
3. **Position Calculation**: Containers position children based on alignment
4. **Recursive Recalculation**: Nested containers update all child positions
5. **Cairo Rendering**: Each widget draws itself using Cairo graphics

### Position Management

Popovers render to a separate Cairo surface, requiring special position handling:

1. StatusBar temporarily moves popover to (0,0) for rendering
2. `RecalculateContainerTree()` recursively updates all widget positions
3. Ensures entire widget tree is positioned correctly relative to popover

This prevents the "widgets rendering off-screen" issue that can occur with nested containers.

### Thread Safety

- Widget properties use `std::recursive_mutex` for thread-safe updates
- Safe to call `SetText()`, `SetTextColor()`, etc. from background threads
- Rendering always happens on the main compositor thread

## Migration from PopoverItem

{{< hint warning >}}
**Deprecated**: The old `PopoverItem` system is deprecated and will be removed in a future version.
{{< /hint >}}

**Old Approach:**
```cpp
popover_->AddItem("󰁹", "Battery", "95%");
popover_->AddSeparator();
popover_->AddItem("󰋋", "Headphones", "100%");
popover_->CalculateSize();
```

**New Approach:**
```cpp
auto vbox = std::make_shared<VBox>();
vbox->SetSpacing(4);

// Row 1
auto row1 = std::make_shared<HBox>();
row1->AddChild(std::make_shared<Label>("󰁹"));
row1->AddChild(std::make_shared<Label>("Battery"));
row1->AddChild(std::make_shared<Label>("95%"));
vbox->AddChild(row1);

// Separator
auto sep = std::make_shared<HBox>();
sep->AddChild(std::make_shared<Label>("────────────────"));
vbox->AddChild(sep);

// Row 2
auto row2 = std::make_shared<HBox>();
row2->AddChild(std::make_shared<Label>("󰋋"));
row2->AddChild(std::make_shared<Label>("Headphones"));
row2->AddChild(std::make_shared<Label>("100%"));
vbox->AddChild(row2);

popover_->SetContent(vbox);
```

## Best Practices

### 1. Clear Before Rebuild

Always clear old content before creating new widget hierarchy:

```cpp
popover_->ClearContent();
auto vbox = std::make_shared<VBox>();
// ... build content ...
popover_->SetContent(vbox);
```

### 2. Use Consistent Spacing

Set spacing on containers for visual consistency:

```cpp
vbox->SetSpacing(4);  // Vertical spacing between rows
hbox->SetSpacing(8);  // Horizontal spacing between items
```

### 3. Font Sizing Hierarchy

Use different font sizes to create visual hierarchy:

```cpp
icon->SetFontSize(16);     // Large for icons
title->SetFontSize(12);    // Medium for titles
detail->SetFontSize(10);   // Small for details
separator->SetFontSize(8); // Tiny for separators
```

### 4. Color Coding

Use colors to convey meaning:

```cpp
// Green for good/charged
label->SetTextColor(0.5, 0.8, 0.5, 1.0);

// Red for warning/low
label->SetTextColor(0.8, 0.3, 0.3, 1.0);

// Gray for disabled/secondary
label->SetTextColor(0.5, 0.5, 0.5, 1.0);
```

## See Also

- [Plugin Development Guide]({{< ref "plugins.md" >}})
- [Architecture Overview]({{< ref "architecture.md" >}})
- [Contributing Guidelines]({{< ref "contributing.md" >}})
