---
title: "Reusable Widgets"
description: "Complete guide to LeviathanDM's reusable widget library for building UI components"
date: 2026-01-01
weight: 30
---

# Reusable Widgets

LeviathanDM 0.0.4 includes a comprehensive library of reusable UI widgets for building custom interfaces, plugins, and UI components. All widgets are thread-safe, Cairo-based, and designed to work seamlessly with the compositor's rendering system.

## Overview

All widgets inherit from the base `Widget` class and provide:
- **Thread-safe operations** with internal mutexes
- **Automatic dirty tracking** for efficient rendering
- **Event handling** for clicks, hover, and keyboard input
- **Flexible styling** with Cairo-based rendering
- **Parent-child relationships** for complex layouts

## Core Widgets

### Label

Displays static or dynamic text with customizable styling.

**Header:** `#include "ui/reusable-widgets/Label.hpp"`

**Basic Usage:**
```cpp
#include "ui/reusable-widgets/Label.hpp"

// Create a label
auto label = std::make_shared<Leviathan::UI::Label>("Hello, World!");

// Update text (thread-safe)
label->SetText("Updated text");

// Styling
label->SetFontSize(14);
label->SetFontFamily("JetBrainsMono Nerd Font");
label->SetTextColor(1.0, 1.0, 1.0, 1.0);  // RGBA: White
label->SetBackgroundColor(0.0, 0.0, 0.0, 0.0);  // Transparent
```

**Methods:**
- `SetText(const std::string& text)` - Update label text
- `GetText()` - Retrieve current text
- `SetFontSize(int size)` - Set font size in points
- `SetFontFamily(const std::string& family)` - Set font family name
- `SetTextColor(double r, double g, double b, double a)` - Set text color (0.0-1.0)
- `SetBackgroundColor(double r, double g, double b, double a)` - Set background color

**Example:**
```cpp
// Clock display
auto time_label = std::make_shared<Leviathan::UI::Label>();
time_label->SetFontSize(16);
time_label->SetTextColor(0.3, 0.8, 1.0, 1.0);  // Cyan

// Update in periodic callback
time_label->SetText(current_time_string);
```

---

### Button

Interactive clickable button with hover effects.

**Header:** `#include "ui/reusable-widgets/Button.hpp"`

**Basic Usage:**
```cpp
#include "ui/reusable-widgets/Button.hpp"

// Create button
auto button = std::make_shared<Leviathan::UI::Button>("Click Me");

// Set click handler
button->SetOnClick([]() {
    LOG_INFO("Button clicked!");
});

// Styling
button->SetTextColor(1.0, 1.0, 1.0, 1.0);          // White text
button->SetBackgroundColor(0.2, 0.2, 0.2, 1.0);    // Dark gray
button->SetHoverColor(0.3, 0.3, 0.3, 1.0);         // Lighter on hover
```

**Methods:**
- `SetText(const std::string& text)` - Set button label
- `SetOnClick(std::function<void()> callback)` - Set click handler
- `SetTextColor(double r, double g, double b, double a)` - Set text color
- `SetBackgroundColor(double r, double g, double b, double a)` - Set background color
- `SetHoverColor(double r, double g, double b, double a)` - Set hover state color
- `SetHovered(bool hovered)` - Manually control hover state

**Example:**
```cpp
// Action button
auto action_btn = std::make_shared<Leviathan::UI::Button>("Reload");
action_btn->SetOnClick([this]() {
    this->ReloadConfiguration();
});
action_btn->SetBackgroundColor(0.3, 0.5, 0.8, 1.0);  // Blue
```

---

### TextField

Text input field with support for standard and outlined variants, text selection, and keyboard shortcuts.

**Header:** `#include "ui/reusable-widgets/TextField.hpp"`

**Variants:**
- `Variant::Standard` - Borderless input (default)
- `Variant::Outlined` - Visible border with focus indication

**Basic Usage:**
```cpp
#include "ui/reusable-widgets/TextField.hpp"

// Create text field
auto text_field = std::make_shared<Leviathan::UI::TextField>(
    "Enter text...",  // Placeholder
    Leviathan::UI::TextField::Variant::Outlined
);

// Set callbacks
text_field->SetOnTextChanged([](const std::string& text) {
    LOG_INFO_FMT("Text changed: {}", text);
});

text_field->SetOnSubmit([](const std::string& text) {
    LOG_INFO_FMT("Submitted: {}", text);
});

text_field->SetOnFocus([]() {
    LOG_INFO("Field focused");
});

text_field->SetOnBlur([]() {
    LOG_INFO("Field lost focus");
});
```

**Methods:**
- `SetText(const std::string& text)` - Set field text
- `GetText()` - Get current text
- `SetPlaceholder(const std::string& placeholder)` - Set placeholder text
- `SetVariant(Variant variant)` - Change variant style
- `SetFontSize(int size)` - Set font size
- `SetMinWidth(int width)` / `SetMaxWidth(int width)` - Control width constraints
- `SetFocused(bool focused)` - Set focus state
- `HandleTextInput(const std::string& text)` - Process text input (UTF-8)
- `HandleKeyPress(uint32_t key, uint32_t modifiers)` - Handle special keys

**Callbacks:**
- `SetOnTextChanged(std::function<void(const std::string&)>)` - Called on any text change
- `SetOnSubmit(std::function<void(const std::string&)>)` - Called on Enter key
- `SetOnFocus(std::function<void()>)` - Called when field gains focus
- `SetOnBlur(std::function<void()>)` - Called when field loses focus

**Keyboard Shortcuts:**
- **Ctrl+A** - Select all text
- **Ctrl+C** - Copy selection to clipboard
- **Ctrl+V** - Paste from clipboard
- **Ctrl+X** - Cut selection to clipboard
- **Home** - Move cursor to start
- **End** - Move cursor to end
- **Left/Right Arrow** - Move cursor
- **Shift+Arrow** - Select text
- **Backspace/Delete** - Delete characters

**Example:**
```cpp
// Search field
auto search = std::make_shared<Leviathan::UI::TextField>(
    "Search applications...",
    Leviathan::UI::TextField::Variant::Standard
);

search->SetOnTextChanged([this](const std::string& query) {
    this->FilterResults(query);
});

search->SetMinWidth(300);
search->SetFontSize(14);
```

---

## Layout Widgets

### Container

Base container class for HBox and VBox. Manages child widgets and their layout.

**Methods:**
- `AddChild(std::shared_ptr<Widget> child)` - Add child widget
- `RemoveChild(std::shared_ptr<Widget> child)` - Remove child widget
- `ClearChildren()` - Remove all children
- `GetChildren()` - Get all child widgets
- `SetSpacing(int spacing)` - Set spacing between children in pixels

---

### HBox

Horizontal container that arranges children in a row.

**Header:** `#include "ui/reusable-widgets/HBox.hpp"`

**Basic Usage:**
```cpp
#include "ui/reusable-widgets/HBox.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include "ui/reusable-widgets/Button.hpp"

// Create horizontal box
auto hbox = std::make_shared<Leviathan::UI::HBox>();
hbox->SetSpacing(8);  // 8px between children
hbox->SetAlign(Leviathan::UI::Align::Center);

// Add children
auto label = std::make_shared<Leviathan::UI::Label>("Status:");
auto button = std::make_shared<Leviathan::UI::Button>("Refresh");

hbox->AddChild(label);
hbox->AddChild(button);
```

**Alignment Options:**
- `Align::Start` - Align to left
- `Align::Center` - Center horizontally
- `Align::End` - Align to right

**Example:**
```cpp
// Status bar with icon and text
auto status_bar = std::make_shared<Leviathan::UI::HBox>();
status_bar->SetSpacing(4);

auto icon = std::make_shared<Leviathan::UI::Label>("🔋");
auto percentage = std::make_shared<Leviathan::UI::Label>("75%");
auto status = std::make_shared<Leviathan::UI::Label>("Charging");

status_bar->AddChild(icon);
status_bar->AddChild(percentage);
status_bar->AddChild(status);
```

---

### VBox

Vertical container that arranges children in a column.

**Header:** `#include "ui/reusable-widgets/VBox.hpp"`

**Basic Usage:**
```cpp
#include "ui/reusable-widgets/VBox.hpp"

// Create vertical box
auto vbox = std::make_shared<Leviathan::UI::VBox>();
vbox->SetSpacing(4);  // 4px between children
vbox->SetAlign(Leviathan::UI::Align::Start);

// Add children vertically
vbox->AddChild(title_label);
vbox->AddChild(content_label);
vbox->AddChild(button);
```

**Alignment Options:**
- `Align::Start` - Align to top
- `Align::Center` - Center vertically
- `Align::End` - Align to bottom

**Example:**
```cpp
// Device list
auto device_list = std::make_shared<Leviathan::UI::VBox>();
device_list->SetSpacing(2);

for (const auto& device : devices) {
    auto device_row = std::make_shared<Leviathan::UI::Label>(
        device.name + ": " + device.status
    );
    device_list->AddChild(device_row);
}
```

---

## Advanced Widgets

### Popover

Floating overlay widget for displaying menus, tooltips, or additional information.

**Header:** `#include "ui/reusable-widgets/Popover.hpp"`

**Basic Usage:**
```cpp
#include "ui/reusable-widgets/Popover.hpp"

// Create popover
auto popover = std::make_shared<Leviathan::UI::Popover>();
popover->SetSize(200, 150);

// Add items
popover->AddItem({
    .text = "Option 1",
    .icon = "📋",
    .detail = "Description",
    .callback = []() { LOG_INFO("Option 1 selected"); },
    .enabled = true
});

popover->AddItem({
    .text = "Option 2",
    .callback = []() { LOG_INFO("Option 2 selected"); }
});

// Show/hide
popover->Show();
popover->Hide();
popover->Toggle();
```

**Methods:**
- `SetPosition(int x, int y)` - Position popover
- `SetSize(int width, int height)` - Set popover dimensions
- `Show()` / `Hide()` / `Toggle()` - Control visibility
- `AddItem(const PopoverItem& item)` - Add menu item
- `ClearItems()` - Remove all items
- `SetContent(std::shared_ptr<Widget> content)` - Set custom widget content

**PopoverItem Structure:**
```cpp
struct PopoverItem {
    std::string text;              // Main text
    std::string icon;              // Icon (emoji or icon font)
    std::string detail;            // Subtitle/description
    std::function<void()> callback; // Click handler
    bool enabled = true;           // Enable/disable item
    bool separator_after = false;  // Show separator after
};
```

**Styling:**
- `SetPadding(int padding)` - Inner padding
- `SetItemHeight(int height)` - Height per item
- `SetFontSize(int size)` - Main text font size
- `SetDetailFontSize(int size)` - Detail text font size
- `SetBackgroundColor(r, g, b, a)` - Background color
- `SetTextColor(r, g, b, a)` - Text color

**Example:**
```cpp
// Battery device menu
auto battery_menu = std::make_shared<Leviathan::UI::Popover>();
battery_menu->SetSize(250, 200);

for (const auto& device : battery_devices) {
    battery_menu->AddItem({
        .text = device.model,
        .icon = device.charging ? "⚡" : "🔋",
        .detail = std::to_string(device.percentage) + "% - " + device.state,
        .callback = [device]() {
            ShowDeviceDetails(device);
        }
    });
}

// Show on widget click
widget->SetOnClick([battery_menu]() {
    battery_menu->Toggle();
});
```

---

### Modal

Full-screen overlay for dialogs and important messages.

**Header:** `#include "ui/reusable-widgets/Modal.hpp"`

See [Modal Documentation](/v0.0.4/en/docs/features/modals) for complete details.

**Quick Example:**
```cpp
#include "ui/reusable-widgets/Modal.hpp"

auto modal = std::make_shared<Leviathan::UI::Modal>();
modal->SetTitle("Confirmation");
modal->SetContent("Are you sure you want to proceed?");

modal->AddButton("Cancel", [modal]() {
    modal->Hide();
});

modal->AddButton("Confirm", [this, modal]() {
    this->PerformAction();
    modal->Hide();
});

modal->Show();
```

---

## Complete Examples

### Status Bar Widget

```cpp
#include "ui/reusable-widgets/HBox.hpp"
#include "ui/reusable-widgets/Label.hpp"

class StatusBarWidget : public Leviathan::UI::PeriodicWidget {
public:
    StatusBarWidget() {
        // Create horizontal layout
        container_ = std::make_shared<Leviathan::UI::HBox>();
        container_->SetSpacing(12);
        
        // Time label
        time_label_ = std::make_shared<Leviathan::UI::Label>();
        time_label_->SetFontSize(14);
        
        // Battery label
        battery_label_ = std::make_shared<Leviathan::UI::Label>();
        battery_label_->SetFontSize(14);
        
        // Add to container
        container_->AddChild(time_label_);
        container_->AddChild(battery_label_);
        
        // Update every second
        SetUpdateInterval(1);
    }
    
    void Update() override {
        // Update time
        auto now = std::time(nullptr);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));
        time_label_->SetText(buffer);
        
        // Update battery
        battery_label_->SetText(GetBatteryStatus());
    }
    
    void Render(cairo_t* cr) override {
        container_->Render(cr);
    }
    
private:
    std::shared_ptr<Leviathan::UI::HBox> container_;
    std::shared_ptr<Leviathan::UI::Label> time_label_;
    std::shared_ptr<Leviathan::UI::Label> battery_label_;
};
```

### Interactive Settings Panel

```cpp
#include "ui/reusable-widgets/VBox.hpp"
#include "ui/reusable-widgets/HBox.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include "ui/reusable-widgets/Button.hpp"
#include "ui/reusable-widgets/TextField.hpp"

class SettingsPanel : public Leviathan::UI::Widget {
public:
    SettingsPanel() {
        // Main vertical layout
        main_layout_ = std::make_shared<Leviathan::UI::VBox>();
        main_layout_->SetSpacing(16);
        
        // Title
        auto title = std::make_shared<Leviathan::UI::Label>("Settings");
        title->SetFontSize(18);
        main_layout_->AddChild(title);
        
        // Name field
        auto name_row = std::make_shared<Leviathan::UI::HBox>();
        name_row->SetSpacing(8);
        
        auto name_label = std::make_shared<Leviathan::UI::Label>("Name:");
        name_field_ = std::make_shared<Leviathan::UI::TextField>(
            "Enter name...",
            Leviathan::UI::TextField::Variant::Outlined
        );
        name_field_->SetMinWidth(200);
        
        name_row->AddChild(name_label);
        name_row->AddChild(name_field_);
        main_layout_->AddChild(name_row);
        
        // Buttons
        auto button_row = std::make_shared<Leviathan::UI::HBox>();
        button_row->SetSpacing(8);
        button_row->SetAlign(Leviathan::UI::Align::End);
        
        auto cancel_btn = std::make_shared<Leviathan::UI::Button>("Cancel");
        cancel_btn->SetOnClick([this]() { this->Hide(); });
        
        auto save_btn = std::make_shared<Leviathan::UI::Button>("Save");
        save_btn->SetBackgroundColor(0.3, 0.6, 0.9, 1.0);  // Blue
        save_btn->SetOnClick([this]() { this->SaveSettings(); });
        
        button_row->AddChild(cancel_btn);
        button_row->AddChild(save_btn);
        main_layout_->AddChild(button_row);
    }
    
    void SaveSettings() {
        std::string name = name_field_->GetText();
        LOG_INFO_FMT("Saving settings: name={}", name);
        // Save to config...
    }
    
    void Render(cairo_t* cr) override {
        main_layout_->Render(cr);
    }
    
private:
    std::shared_ptr<Leviathan::UI::VBox> main_layout_;
    std::shared_ptr<Leviathan::UI::TextField> name_field_;
};
```

---

## Best Practices

### Thread Safety

All widgets are thread-safe and can be updated from background threads:

```cpp
// Safe to call from any thread
std::thread updater([label]() {
    while (running) {
        label->SetText(GetUpdatedData());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
});
```

### Performance

- Widgets use dirty tracking - only re-render when changed
- Use `CalculateSize()` before rendering for proper layout
- Cache widget references instead of recreating

```cpp
// Good: Cache widget
auto label = std::make_shared<Label>();
void Update() {
    label->SetText(new_text);  // Just updates, doesn't recreate
}

// Bad: Recreate every time
void Update() {
    label = std::make_shared<Label>(new_text);  // Allocates memory
}
```

### Memory Management

Use `std::shared_ptr` for all widgets:

```cpp
auto container = std::make_shared<VBox>();
auto child = std::make_shared<Label>("Text");

container->AddChild(child);
// Both container and child manage their lifetime correctly
```

### Event Handling

Coordinate transformation is handled automatically for containers:

```cpp
// Container automatically transforms click coordinates for children
auto hbox = std::make_shared<HBox>();
auto button = std::make_shared<Button>("Click");

button->SetOnClick([]() {
    // This works correctly even inside nested containers
    LOG_INFO("Button clicked!");
});

hbox->AddChild(button);
```

---

## Widget Lifecycle

1. **Creation** - Widget constructed with initial properties
2. **Configuration** - Set colors, fonts, callbacks, etc.
3. **Layout** - `CalculateSize()` computes dimensions
4. **Rendering** - `Render()` draws to Cairo context
5. **Events** - `HandleClick()`, `HandleKeyPress()`, etc.
6. **Updates** - Properties changed, triggers dirty flag
7. **Cleanup** - Shared pointers handle destruction

---

## API Reference

### Common Widget Methods

All widgets inherit these from `BaseWidget`:

```cpp
// Size and position
void SetWidth(int width);
void SetHeight(int height);
void SetSize(int width, int height);
int GetWidth() const;
int GetHeight() const;

// Visibility
void SetVisible(bool visible);
bool IsVisible() const;

// Parent-child relationships
void SetParent(Widget* parent);
Widget* GetParent() const;

// Dirty tracking
void MarkDirty();
bool IsDirty() const;
void ClearDirty();

// Rendering
virtual void Render(cairo_t* cr) = 0;
virtual void CalculateSize(int available_width, int available_height);

// Event handling
virtual bool HandleClick(int x, int y);
virtual bool HandleKeyPress(uint32_t key, uint32_t modifiers);
virtual bool HandleHover(int x, int y);
```

---

## See Also

- [Plugin Development Guide](/v0.0.4/en/docs/development/plugins)
- [Modal System](/v0.0.4/en/docs/features/modals)
- [StatusBar Widgets](/v0.0.4/en/docs/features/statusbar)
- [Custom UI Components](/v0.0.4/en/docs/development/custom-ui)
