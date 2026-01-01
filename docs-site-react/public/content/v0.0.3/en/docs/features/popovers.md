# Popover System

**Available since:** v0.0.3

The popover system in LeviathanDM provides context menus and tooltips that appear near widgets when clicked. Popovers use the widget-based architecture, allowing you to build complex layouts with composable components.

## Overview

Popovers are temporary UI elements that:
- Appear on top of all other content (Top layer)
- Display additional information or actions
- Automatically position near the trigger widget
- Dismiss when clicking outside
- Support scrolling for long content

**Common Use Cases:**
- Battery status details
- Clock with calendar
- Network connection info
- Volume controls
- Quick actions menu

---

## Architecture

### Widget-Based Design

Unlike traditional popover systems that require manual drawing, LeviathanDM popovers accept widget hierarchies:

```cpp
// Build popover content using widgets
auto vbox = std::make_shared<VBox>();
vbox->AddChild(std::make_shared<Label>("Battery Status"));
vbox->AddChild(std::make_shared<Label>("82% - Charging"));

popover->SetContent(vbox);
```

**Benefits:**
- **Composability:** Mix VBox, HBox, Label, and custom widgets
- **Consistency:** Same widgets used everywhere in the UI
- **Flexibility:** Easy to add buttons, icons, sliders, etc.
- **Maintainability:** Familiar widget patterns

### Rendering Pipeline

```
User clicks widget
    ↓
Widget shows popover
    ↓
LayerManager renders popover to Top layer
    ↓
Popover positions itself near widget
    ↓
Widget tree renders to Cairo surface
    ↓
Surface composited on screen
```

**Key Points:**
- Popovers render to a separate surface
- Positioned using absolute coordinates
- Widget tree is recursively calculated
- Surface is reused between renders

---

## Creating Popovers

### Step 1: Implement IPopoverProvider

Widgets that want popover functionality implement the `IPopoverProvider` interface:

```cpp
#include "ui/IPopoverProvider.hpp"
#include "ui/reusable-widgets/Popover.hpp"

class MyWidget : public PeriodicWidget, public IPopoverProvider {
public:
    MyWidget() {
        popover_ = std::make_shared<Popover>();
        UpdatePopoverContent();
    }
    
    // IPopoverProvider interface
    std::shared_ptr<Popover> GetPopover() const override {
        return popover_;
    }
    
    bool HasPopover() const override {
        return popover_ != nullptr;
    }

protected:
    void OnUpdate() override {
        // Update main widget display
        UpdatePopoverContent();  // Keep popover in sync
    }

private:
    std::shared_ptr<Popover> popover_;
    void UpdatePopoverContent();
};
```

### Step 2: Build Widget Content

Create a widget hierarchy for the popover:

```cpp
void MyWidget::UpdatePopoverContent() {
    popover_->ClearContent();
    
    // Create container
    auto vbox = std::make_shared<VBox>();
    vbox->SetSpacing(8);
    vbox->SetPadding(12);
    
    // Add header
    auto header = std::make_shared<Label>("System Information");
    header->SetFontSize(14);
    header->SetBold(true);
    vbox->AddChild(header);
    
    // Add separator
    auto separator = std::make_shared<Label>("───────────────");
    separator->SetColor(0.5, 0.5, 0.5);
    vbox->AddChild(separator);
    
    // Add info rows
    auto row1 = std::make_shared<HBox>();
    row1->SetSpacing(12);
    row1->AddChild(std::make_shared<Label>("CPU:"));
    row1->AddChild(std::make_shared<Label>("45%"));
    vbox->AddChild(row1);
    
    auto row2 = std::make_shared<HBox>();
    row2->SetSpacing(12);
    row2->AddChild(std::make_shared<Label>("Memory:"));
    row2->AddChild(std::make_shared<Label>("8.2 GB"));
    vbox->AddChild(row2);
    
    // Set as popover content
    popover_->SetContent(vbox);
}
```

### Step 3: Toggle on Click

The base widget system automatically handles showing/hiding popovers on click:

```cpp
// In BaseWidget::HandleClick()
if (HasPopover()) {
    auto popover = GetPopover();
    if (popover->IsVisible()) {
        popover->Hide();
    } else {
        // Position near widget
        int abs_x = GetAbsoluteX();
        int abs_y = GetAbsoluteY();
        popover->SetPosition(abs_x, abs_y + GetHeight() + 4);
        popover->Show();
    }
    return true;
}
```

**Automatic Behavior:**
- Click widget → popover appears
- Click again → popover hides
- Click outside → popover hides

---

## Popover Positioning

### Automatic Positioning

Popovers automatically position themselves near the trigger widget:

```cpp
// Widget calculates position
int abs_x = GetAbsoluteX();
int abs_y = GetAbsoluteY();

// Position below widget with 4px gap
popover_->SetPosition(abs_x, abs_y + GetHeight() + 4);
```

### Custom Positioning

You can override positioning for special cases:

```cpp
// Position above widget instead
popover_->SetPosition(abs_x, abs_y - popover_->GetHeight() - 4);

// Position to the right
popover_->SetPosition(abs_x + GetWidth() + 4, abs_y);

// Center horizontally
int center_x = abs_x + (GetWidth() - popover_->GetWidth()) / 2;
popover_->SetPosition(center_x, abs_y + GetHeight() + 4);
```

### Screen Edge Detection (Future)

Currently, popovers may go off-screen. Future versions will include:
- Automatic flip (show above if below would clip)
- Horizontal constraint (keep within screen bounds)
- Smart positioning strategies

---

## Widget Composition

### Layout Containers

#### VBox (Vertical Box)
Stack widgets vertically:

```cpp
auto vbox = std::make_shared<VBox>();
vbox->SetSpacing(8);           // 8px between children
vbox->SetPadding(12);          // 12px around all sides
vbox->SetAlignment(CENTER);    // LEFT, CENTER, RIGHT

vbox->AddChild(widget1);
vbox->AddChild(widget2);
vbox->AddChild(widget3);
```

#### HBox (Horizontal Box)
Arrange widgets horizontally:

```cpp
auto hbox = std::make_shared<HBox>();
hbox->SetSpacing(12);
hbox->SetPadding(8);
hbox->SetAlignment(MIDDLE);    // TOP, MIDDLE, BOTTOM

hbox->AddChild(icon);
hbox->AddChild(label);
hbox->AddChild(value);
```

### Text Labels

```cpp
auto label = std::make_shared<Label>("Text");
label->SetFontSize(12);
label->SetBold(true);
label->SetColor(1.0, 1.0, 1.0);      // RGB (white)
label->SetColor(0.8, 0.8, 0.8, 0.9); // RGBA (semi-transparent)
```

### Nested Layouts

Combine containers for complex layouts:

```cpp
auto main_vbox = std::make_shared<VBox>();
main_vbox->SetSpacing(4);

// Header row
auto header_row = std::make_shared<HBox>();
header_row->AddChild(std::make_shared<Label>("🔋"));
header_row->AddChild(std::make_shared<Label>("Battery Status"));
main_vbox->AddChild(header_row);

// Info rows
auto info_row1 = std::make_shared<HBox>();
info_row1->AddChild(std::make_shared<Label>("Level:"));
info_row1->AddChild(std::make_shared<Label>("82%"));
main_vbox->AddChild(info_row1);

auto info_row2 = std::make_shared<HBox>();
info_row2->AddChild(std::make_shared<Label>("Status:"));
info_row2->AddChild(std::make_shared<Label>("Charging"));
main_vbox->AddChild(info_row2);

popover_->SetContent(main_vbox);
```

**Result:**
```
┌─────────────────────┐
│ 🔋 Battery Status   │
│ Level:      82%     │
│ Status:     Charging│
└─────────────────────┘
```

---

## Real-World Example: Battery Widget

Complete implementation from the battery widget plugin:

```cpp
// BatteryWidget.hpp
class BatteryWidget : public PeriodicWidget, public IPopoverProvider {
public:
    BatteryWidget();
    
    std::shared_ptr<Popover> GetPopover() const override { 
        return popover_; 
    }
    
    bool HasPopover() const override { 
        return popover_ != nullptr; 
    }

protected:
    void OnUpdate() override;

private:
    std::shared_ptr<Popover> popover_;
    int percentage_ = 0;
    bool charging_ = false;
    int time_remaining_ = 0;
    
    void UpdatePopoverContent();
    void ReadBatteryStatus();
};

// BatteryWidget.cpp
BatteryWidget::BatteryWidget() 
    : PeriodicWidget(5000) {  // Update every 5 seconds
    popover_ = std::make_shared<Popover>();
    OnUpdate();
}

void BatteryWidget::OnUpdate() {
    ReadBatteryStatus();
    UpdatePopoverContent();
    RequestRedraw();
}

void BatteryWidget::UpdatePopoverContent() {
    popover_->ClearContent();
    
    auto vbox = std::make_shared<VBox>();
    vbox->SetSpacing(6);
    vbox->SetPadding(12);
    
    // Header
    auto header = std::make_shared<HBox>();
    header->SetSpacing(8);
    header->AddChild(std::make_shared<Label>("🔋"));
    header->AddChild(std::make_shared<Label>("Battery Details"));
    vbox->AddChild(header);
    
    // Separator
    auto sep = std::make_shared<Label>("─────────────────");
    sep->SetColor(0.4, 0.4, 0.4);
    vbox->AddChild(sep);
    
    // Status row
    auto status_row = std::make_shared<HBox>();
    status_row->SetSpacing(12);
    status_row->AddChild(std::make_shared<Label>("Status:"));
    status_row->AddChild(std::make_shared<Label>(
        charging_ ? "⚡ Charging" : "🔌 Discharging"
    ));
    vbox->AddChild(status_row);
    
    // Level row
    auto level_row = std::make_shared<HBox>();
    level_row->SetSpacing(12);
    level_row->AddChild(std::make_shared<Label>("Level:"));
    level_row->AddChild(std::make_shared<Label>(
        std::to_string(percentage_) + "%"
    ));
    vbox->AddChild(level_row);
    
    // Time remaining row
    if (time_remaining_ > 0) {
        auto time_row = std::make_shared<HBox>();
        time_row->SetSpacing(12);
        time_row->AddChild(std::make_shared<Label>(
            charging_ ? "Time to full:" : "Time remaining:"
        ));
        
        int hours = time_remaining_ / 60;
        int mins = time_remaining_ % 60;
        time_row->AddChild(std::make_shared<Label>(
            std::to_string(hours) + "h " + std::to_string(mins) + "m"
        ));
        vbox->AddChild(time_row);
    }
    
    popover_->SetContent(vbox);
}

void BatteryWidget::ReadBatteryStatus() {
    // Read from /sys/class/power_supply/BAT0/
    // ... implementation details ...
}
```

---

## Styling Tips

### Colors

Use consistent colors for better UX:

```cpp
// Headers
label->SetColor(1.0, 1.0, 1.0);  // Bright white

// Body text
label->SetColor(0.9, 0.9, 0.9);  // Slightly dimmed

// Secondary text
label->SetColor(0.7, 0.7, 0.7);  // Gray

// Separators
label->SetColor(0.4, 0.4, 0.4);  // Dark gray

// Accents (warnings, errors)
label->SetColor(1.0, 0.6, 0.2);  // Orange
label->SetColor(1.0, 0.3, 0.3);  // Red
```

### Spacing

Consistent spacing improves readability:

```cpp
// Large outer padding
vbox->SetPadding(16);

// Medium spacing between sections
vbox->SetSpacing(12);

// Small spacing within rows
hbox->SetSpacing(8);

// Tight spacing for related items
hbox->SetSpacing(4);
```

### Typography

```cpp
// Headers: Bold, larger
auto header = std::make_shared<Label>("Title");
header->SetBold(true);
header->SetFontSize(14);

// Body: Normal weight, standard size
auto body = std::make_shared<Label>("Text");
body->SetFontSize(11);

// Small text: Smaller size for secondary info
auto small = std::make_shared<Label>("Details");
small->SetFontSize(9);
small->SetColor(0.7, 0.7, 0.7);
```

---

## Performance Considerations

### Update Frequency

Only update popovers when necessary:

```cpp
void OnUpdate() override {
    bool changed = ReadNewData();
    
    // Only rebuild popover if data changed
    if (changed || !popover_content_built_) {
        UpdatePopoverContent();
        popover_content_built_ = true;
    }
}
```

### Large Content

For popovers with lots of content, consider:

1. **Lazy loading:** Build content only when popover opens
2. **Scrolling:** Use ScrollView for long lists (v0.0.4+)
3. **Pagination:** Split content across multiple views
4. **Caching:** Reuse widget instances when possible

### Memory

Popovers maintain their widget tree even when hidden. For memory-constrained systems:

```cpp
// Clear content when hidden
void OnPopoverHidden() {
    popover_->ClearContent();
    popover_content_built_ = false;
}

// Rebuild when shown
void OnPopoverShown() {
    UpdatePopoverContent();
}
```

---

## Lifecycle

### States

1. **Created** - Popover object exists
2. **Built** - Widget content added
3. **Shown** - Visible on screen
4. **Hidden** - Not visible but content retained
5. **Cleared** - Content removed

### State Transitions

```cpp
// Create
popover_ = std::make_shared<Popover>();  // → Created

// Build content
popover_->SetContent(vbox);  // → Built

// Show
popover_->Show();  // → Shown

// Hide (content retained)
popover_->Hide();  // → Hidden

// Clear and rebuild
popover_->ClearContent();  // → Cleared
popover_->SetContent(new_vbox);  // → Built
```

---

## Debugging

### Common Issues

#### Popover doesn't appear

**Check:**
1. Widget implements `IPopoverProvider`
2. `HasPopover()` returns true
3. `GetPopover()` returns valid pointer
4. Content was set: `popover_->SetContent(widget)`

#### Content renders at wrong position

**Solution:** Ensure container positions are recalculated:

```cpp
// This is handled automatically, but if you see issues:
popover_->RecalculateLayout();
```

#### Text is cut off

**Cause:** Widget size calculation didn't account for text width.

**Solution:** Increase padding or set explicit widths:

```cpp
vbox->SetPadding(16);  // More padding
label->SetMinWidth(200);  // Minimum width
```

#### Popover flickers

**Cause:** Rebuilding content too frequently.

**Solution:** Cache and only rebuild when data changes.

---

## Migration from Legacy System

### Old PopoverItem System (deprecated)

```cpp
// Old way (v0.0.2 and earlier)
popover_->AddItem("Icon", "Title", "Value");
popover_->AddSeparator();
popover_->CalculateSize();
```

### New Widget-Based System (v0.0.3+)

```cpp
// New way
auto vbox = std::make_shared<VBox>();

auto row = std::make_shared<HBox>();
row->AddChild(std::make_shared<Label>("Icon"));
row->AddChild(std::make_shared<Label>("Title"));
row->AddChild(std::make_shared<Label>("Value"));
vbox->AddChild(row);

auto sep = std::make_shared<Label>("─────────");
vbox->AddChild(sep);

popover_->SetContent(vbox);
```

**Benefits of migration:**
- More flexible layouts
- Better text rendering
- Consistent with rest of UI
- Easier to maintain

---

## Future Enhancements

Planned improvements for the popover system:

- **Animations:** Fade in/out, slide effects
- **Smart positioning:** Auto-flip to avoid screen edges
- **Interactive content:** Buttons, sliders, inputs
- **Scrolling:** Integrated ScrollView support (v0.0.4+)
- **Theming:** Configurable appearance
- **Nested popovers:** Popovers within popovers

---

## See Also

- [Widget System](/docs/development/widget-system) - Complete widget architecture
- [Status Bar](/docs/features/status-bar) - Using popovers in status bar widgets
- [Modal System](/docs/features/modals) - Similar to popovers but center-screen
- [Plugin Development](/docs/development/plugins) - Creating custom popover widgets
