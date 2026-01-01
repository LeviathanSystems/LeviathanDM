# Modal System

**Available since:** v0.0.4

Modals are full-screen overlay dialogs that display important information or require user interaction. Unlike popovers that appear near a widget, modals are centered on screen with a dimmed background.

## Overview

The modal system provides:
- **Center-screen positioning** - Always visible and prominent
- **Background dimming** - Focus attention on modal content
- **Keyboard shortcuts** - Easy to trigger and dismiss
- **Scrollable content** - Handle large amounts of information
- **Widget composition** - Build complex layouts easily
- **Event handling** - Clicks, hovers, and scrolling

**Common Use Cases:**
- Keybinding help (Super+?)
- Settings panels
- Confirmation dialogs
- Information displays
- Application launchers
- File pickers

---

## Architecture

### Rendering Pipeline

```
Action triggered (keybinding)
    ↓
Modal created and shown
    ↓
LayerManager detects visible modal
    ↓
Background dimmed (0.6 opacity)
    ↓
Modal renders to Top layer
    ↓
Widget tree renders content
    ↓
Events forwarded to modal
```

**Key Characteristics:**
- Rendered by `LayerManager` on Top layer
- Blocks interaction with windows below
- Dismisses on Escape or background click
- Can contain any widget hierarchy
- Supports scrolling with ScrollView

### Coordinate System

Modals use proper hierarchical coordinates:

```cpp
// Modal establishes origin at its position
cairo_save(cr);
cairo_translate(cr, modal_x, modal_y);

// Children render in local space
content_widget->Render(cr);  // At (0, 0) relative to modal

cairo_restore(cr);
```

**Benefits:**
- Children use relative coordinates
- No position accumulation bugs
- Easy to nest containers
- Correct event handling

---

## Creating Modals

### Step 1: Extend BaseModal

```cpp
#include "ui/reusable-widgets/BaseModal.hpp"

class MyModal : public BaseModal {
public:
    MyModal() : BaseModal(600, 400) {  // Width, height
        BuildContent();
    }

protected:
    void BuildContent() override;
    void OnClose() override;
};
```

### Step 2: Build Widget Content

```cpp
void MyModal::BuildContent() {
    auto vbox = std::make_shared<VBox>();
    vbox->SetSpacing(12);
    vbox->SetPadding(20);
    
    // Title
    auto title = std::make_shared<Label>("My Modal");
    title->SetBold(true);
    title->SetFontSize(16);
    vbox->AddChild(title);
    
    // Content
    auto content = std::make_shared<Label>("This is modal content.");
    vbox->AddChild(content);
    
    // Footer
    auto footer = std::make_shared<Label>("Press ESC to close");
    footer->SetColor(0.6, 0.6, 0.6);
    footer->SetFontSize(10);
    vbox->AddChild(footer);
    
    SetContent(vbox);
}
```

### Step 3: Show and Hide

```cpp
// Show modal
auto modal = std::make_shared<MyModal>();
layer_manager->ShowModal(modal);

// Hide modal (usually done by pressing ESC or clicking outside)
layer_manager->HideModal();
```

---

## Built-in Example: Keybindings Help

The keybindings help modal (Super+?) demonstrates best practices:

```cpp
class KeybindingHelpModal : public BaseModal {
public:
    KeybindingHelpModal(const std::vector<KeyBinding>& bindings)
        : BaseModal(700, 500), bindings_(bindings) {
        BuildContent();
    }

protected:
    void BuildContent() override {
        // Main container
        auto main_vbox = std::make_shared<VBox>();
        main_vbox->SetSpacing(16);
        main_vbox->SetPadding(24);
        
        // Header
        auto header = std::make_shared<HBox>();
        header->SetSpacing(12);
        
        auto icon = std::make_shared<Label>("⌨️");
        icon->SetFontSize(20);
        header->AddChild(icon);
        
        auto title = std::make_shared<Label>("Keyboard Shortcuts");
        title->SetBold(true);
        title->SetFontSize(18);
        header->AddChild(title);
        
        main_vbox->AddChild(header);
        
        // Keybinding rows
        for (const auto& binding : bindings_) {
            auto row = std::make_shared<HBox>();
            row->SetSpacing(24);
            
            // Key combination
            auto key_label = std::make_shared<Label>(binding.key_combo);
            key_label->SetFontSize(12);
            key_label->SetColor(0.4, 0.7, 1.0);  // Blue
            row->AddChild(key_label);
            
            // Description
            auto desc_label = std::make_shared<Label>(binding.description);
            desc_label->SetFontSize(12);
            row->AddChild(desc_label);
            
            main_vbox->AddChild(row);
        }
        
        // Footer
        auto footer = std::make_shared<Label>("Press ESC or click outside to close");
        footer->SetFontSize(10);
        footer->SetColor(0.6, 0.6, 0.6);
        main_vbox->AddChild(footer);
        
        // Wrap in ScrollView for long lists
        auto scroll_view = std::make_shared<ScrollView>();
        scroll_view->SetChild(main_vbox);
        scroll_view->SetScrollbarColor(0.6, 0.6, 0.6, 0.7);
        scroll_view->ShowScrollbar(true);
        
        SetContent(scroll_view);
    }

private:
    std::vector<KeyBinding> bindings_;
};
```

**Usage:**
```cpp
// Triggered by Super+? keybinding
void ShowKeybindingHelp() {
    auto modal = std::make_shared<KeybindingHelpModal>(keybindings_);
    layer_manager_->ShowModal(modal);
}
```

---

## Scrollable Content

### Using ScrollView

For modals with lots of content, wrap in a `ScrollView`:

```cpp
void MyModal::BuildContent() {
    // Build your content
    auto content_vbox = std::make_shared<VBox>();
    // ... add many children ...
    
    // Wrap in ScrollView
    auto scroll_view = std::make_shared<ScrollView>();
    scroll_view->SetChild(content_vbox);
    scroll_view->SetScrollbarColor(0.6, 0.6, 0.6, 0.7);
    scroll_view->ShowScrollbar(true);
    
    // Set ScrollView as modal content
    SetContent(scroll_view);
}
```

**Features:**
- **Mouse wheel scrolling** - Scroll up/down naturally
- **Visible scrollbar** - Shows position and total size
- **Clipping** - Content outside viewport is hidden
- **Smooth scrolling** - Instant response to input

**Configuration:**
```cpp
// Scrollbar appearance
scroll_view->SetScrollbarColor(r, g, b, a);
scroll_view->ShowScrollbar(true);  // or false to hide

// Scrollbar position (right side by default)
scroll_view->SetScrollbarWidth(8);  // Pixels
```

### Scroll Behavior

The scroll system transforms events correctly:

```cpp
// User scrolls mouse wheel
HandleScroll(screen_x, screen_y, delta_x, delta_y)
    ↓
// Modal transforms to local coordinates
local_x = screen_x - modal_x;
local_y = screen_y - modal_y;
    ↓
// ScrollView checks if event is inside viewport
if (IsPointInside(local_x, local_y)) {
    scroll_offset_ += delta_y * scroll_speed;
    ClampScrollOffset();
    return true;  // Handled
}
```

---

## Sizing and Positioning

### Fixed Size

Specify exact dimensions:

```cpp
MyModal() : BaseModal(800, 600) {  // 800px wide, 600px tall
    BuildContent();
}
```

### Dynamic Size

Calculate size based on content:

```cpp
MyModal() : BaseModal(0, 0) {  // Will be calculated
    BuildContent();
    
    // Calculate based on content
    int content_width = content_widget_->GetWidth() + padding * 2;
    int content_height = content_widget_->GetHeight() + padding * 2;
    
    SetSize(content_width, content_height);
}
```

### Centering

Modals are automatically centered by LayerManager:

```cpp
// LayerManager calculates center position
int modal_x = (screen_width - modal_width) / 2;
int modal_y = (screen_height - modal_height) / 2;

modal->SetPosition(modal_x, modal_y);
```

### Max Size

Prevent modals from exceeding screen bounds:

```cpp
int max_width = screen_width * 0.8;   // 80% of screen
int max_height = screen_height * 0.8;

int modal_width = std::min(desired_width, max_width);
int modal_height = std::min(desired_height, max_height);

SetSize(modal_width, modal_height);
```

---

## Event Handling

### Click Events

Clicks are transformed to modal-local coordinates:

```cpp
bool MyModal::HandleClick(int x, int y) {
    // x, y are already in modal-local coordinates
    
    if (close_button_->Contains(x, y)) {
        Close();
        return true;
    }
    
    // Forward to content
    return content_widget_->HandleClick(x, y);
}
```

### Hover Events

```cpp
bool MyModal::HandleHover(int x, int y) {
    // Update hover state
    if (close_button_->Contains(x, y)) {
        close_button_->SetHovered(true);
    } else {
        close_button_->SetHovered(false);
    }
    
    return content_widget_->HandleHover(x, y);
}
```

### Scroll Events

```cpp
bool MyModal::HandleScroll(int x, int y, double dx, double dy) {
    // Forward to content (usually ScrollView)
    return content_widget_->HandleScroll(x, y, dx, dy);
}
```

### Keyboard Events

```cpp
bool MyModal::HandleKey(uint32_t key, bool pressed) {
    if (pressed && key == KEY_ESCAPE) {
        Close();
        return true;
    }
    
    // Handle other keys
    return false;
}
```

---

## Styling

### Background Dim

The modal background is automatically dimmed:

```cpp
// LayerManager dims background
cairo_set_source_rgba(cr, 0, 0, 0, 0.6);  // 60% black
cairo_paint(cr);
```

**Customization** (future):
```cpp
modal->SetBackgroundDimAmount(0.7);  // 70% dim
modal->SetBackgroundColor(0.1, 0.1, 0.2, 0.6);  // Dark blue tint
```

### Modal Appearance

```cpp
void MyModal::Render(cairo_t* cr) {
    // Background
    cairo_set_source_rgba(cr, 0.15, 0.15, 0.15, 0.95);
    cairo_rectangle(cr, 0, 0, width_, height_);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgba(cr, 0.3, 0.5, 0.8, 1.0);
    cairo_set_line_width(cr, 2);
    cairo_rectangle(cr, 0, 0, width_, height_);
    cairo_stroke(cr);
    
    // Render content
    if (content_widget_) {
        content_widget_->Render(cr);
    }
}
```

### Content Styling

Use consistent colors and spacing:

```cpp
// Title: Bold, large, bright
auto title = std::make_shared<Label>("Title");
title->SetBold(true);
title->SetFontSize(18);
title->SetColor(1.0, 1.0, 1.0);

// Body: Normal, medium
auto body = std::make_shared<Label>("Content");
body->SetFontSize(12);
body->SetColor(0.9, 0.9, 0.9);

// Footer: Small, dimmed
auto footer = std::make_shared<Label>("Hint");
footer->SetFontSize(10);
footer->SetColor(0.6, 0.6, 0.6);

// Spacing
vbox->SetSpacing(16);      // Between sections
vbox->SetPadding(24);      // Around edges
hbox->SetSpacing(12);      // Within rows
```

---

## Lifecycle Management

### States

1. **Created** - Modal object instantiated
2. **Built** - Content widgets added
3. **Shown** - Visible on screen
4. **Hidden** - Dismissed but object exists
5. **Destroyed** - Object deleted

### Showing Modals

```cpp
// Create modal
auto modal = std::make_shared<MyModal>();

// Show via LayerManager
layer_manager->ShowModal(modal);

// Or via Server
server->ShowModal(modal);
```

### Hiding Modals

```cpp
// Hide current modal
layer_manager->HideModal();

// Or from within modal
void MyModal::OnClose() {
    // Cleanup before hiding
    parent_->HideModal();
}
```

### Auto-dismiss

Modals automatically dismiss on:
- **Escape key** - Universal close shortcut
- **Background click** - Clicking outside modal area
- **Explicit close** - Custom close button or action

```cpp
// Check if click is outside modal
bool LayerManager::HandleModalClick(int x, int y) {
    if (!current_modal_) return false;
    
    // Get modal bounds
    int mx = current_modal_->GetX();
    int my = current_modal_->GetY();
    int mw = current_modal_->GetWidth();
    int mh = current_modal_->GetHeight();
    
    // Click outside?
    if (x < mx || x > mx + mw || y < my || y > my + mh) {
        HideModal();  // Dismiss
        return true;
    }
    
    // Forward to modal
    return current_modal_->HandleClick(x - mx, y - my);
}
```

---

## Advanced Examples

### Confirmation Dialog

```cpp
class ConfirmationModal : public BaseModal {
public:
    using Callback = std::function<void(bool)>;
    
    ConfirmationModal(const std::string& message, Callback callback)
        : BaseModal(400, 200), message_(message), callback_(callback) {
        BuildContent();
    }

protected:
    void BuildContent() override {
        auto vbox = std::make_shared<VBox>();
        vbox->SetSpacing(20);
        vbox->SetPadding(30);
        
        // Message
        auto msg_label = std::make_shared<Label>(message_);
        msg_label->SetFontSize(14);
        vbox->AddChild(msg_label);
        
        // Buttons
        auto button_row = std::make_shared<HBox>();
        button_row->SetSpacing(16);
        
        auto yes_btn = std::make_shared<Button>("Yes");
        yes_btn->OnClick([this]() {
            callback_(true);
            Close();
        });
        button_row->AddChild(yes_btn);
        
        auto no_btn = std::make_shared<Button>("No");
        no_btn->OnClick([this]() {
            callback_(false);
            Close();
        });
        button_row->AddChild(no_btn);
        
        vbox->AddChild(button_row);
        SetContent(vbox);
    }

private:
    std::string message_;
    Callback callback_;
};

// Usage:
void ConfirmShutdown() {
    auto modal = std::make_shared<ConfirmationModal>(
        "Are you sure you want to shutdown?",
        [](bool confirmed) {
            if (confirmed) {
                Shutdown();
            }
        }
    );
    layer_manager->ShowModal(modal);
}
```

### Settings Panel

```cpp
class SettingsModal : public BaseModal {
public:
    SettingsModal() : BaseModal(800, 600) {
        BuildContent();
    }

protected:
    void BuildContent() override {
        auto main_vbox = std::make_shared<VBox>();
        main_vbox->SetSpacing(20);
        main_vbox->SetPadding(30);
        
        // Title
        auto title = std::make_shared<Label>("⚙️ Settings");
        title->SetBold(true);
        title->SetFontSize(20);
        main_vbox->AddChild(title);
        
        // Section: Display
        main_vbox->AddChild(CreateSection("Display"));
        main_vbox->AddChild(CreateSetting("Border Width", "2px"));
        main_vbox->AddChild(CreateSetting("Gaps", "8px"));
        
        // Section: Behavior
        main_vbox->AddChild(CreateSection("Behavior"));
        main_vbox->AddChild(CreateSetting("Focus Follows Mouse", "No"));
        main_vbox->AddChild(CreateSetting("Wrap Focus", "Yes"));
        
        // Wrap in scroll view
        auto scroll = std::make_shared<ScrollView>();
        scroll->SetChild(main_vbox);
        scroll->ShowScrollbar(true);
        
        SetContent(scroll);
    }
    
    std::shared_ptr<Widget> CreateSection(const std::string& name) {
        auto label = std::make_shared<Label>(name);
        label->SetBold(true);
        label->SetFontSize(14);
        label->SetColor(0.4, 0.7, 1.0);
        return label;
    }
    
    std::shared_ptr<Widget> CreateSetting(
        const std::string& name,
        const std::string& value
    ) {
        auto row = std::make_shared<HBox>();
        row->SetSpacing(20);
        
        auto name_label = std::make_shared<Label>(name);
        row->AddChild(name_label);
        
        auto value_label = std::make_shared<Label>(value);
        value_label->SetColor(0.7, 0.7, 0.7);
        row->AddChild(value_label);
        
        return row;
    }
};
```

### File Picker (Concept)

```cpp
class FilePickerModal : public BaseModal {
public:
    using Callback = std::function<void(const std::string&)>;
    
    FilePickerModal(const std::string& directory, Callback callback)
        : BaseModal(600, 500), current_dir_(directory), callback_(callback) {
        LoadDirectory();
        BuildContent();
    }

protected:
    void BuildContent() override {
        auto vbox = std::make_shared<VBox>();
        vbox->SetSpacing(12);
        vbox->SetPadding(20);
        
        // Path display
        auto path_label = std::make_shared<Label>(current_dir_);
        path_label->SetFontSize(10);
        path_label->SetColor(0.6, 0.6, 0.6);
        vbox->AddChild(path_label);
        
        // File list
        for (const auto& file : files_) {
            auto file_row = CreateFileRow(file);
            vbox->AddChild(file_row);
        }
        
        // Wrap in scroll view
        auto scroll = std::make_shared<ScrollView>();
        scroll->SetChild(vbox);
        scroll->ShowScrollbar(true);
        
        SetContent(scroll);
    }
    
    std::shared_ptr<Widget> CreateFileRow(const FileInfo& file) {
        auto row = std::make_shared<HBox>();
        row->SetSpacing(12);
        
        // Icon
        auto icon = std::make_shared<Label>(file.is_dir ? "📁" : "📄");
        row->AddChild(icon);
        
        // Name
        auto name = std::make_shared<Label>(file.name);
        row->AddChild(name);
        
        // Make clickable
        row->OnClick([this, file]() {
            if (file.is_dir) {
                current_dir_ = file.path;
                LoadDirectory();
                BuildContent();
            } else {
                callback_(file.path);
                Close();
            }
        });
        
        return row;
    }
    
    void LoadDirectory() {
        files_.clear();
        // ... scan directory ...
    }

private:
    std::string current_dir_;
    std::vector<FileInfo> files_;
    Callback callback_;
};
```

---

## Debugging

### Common Issues

#### Modal doesn't appear

**Check:**
1. Modal was added to LayerManager: `layer_manager->ShowModal(modal)`
2. Content was set: `SetContent(widget)`
3. Size is non-zero: `BaseModal(width, height)` with valid dimensions
4. LayerManager is rendering: `RenderModals()` called in render loop

#### Content renders at wrong position

**Cause:** Coordinate transformation issue.

**Solution:** Ensure using hierarchical coordinates. Container should use `cairo_translate()`.

#### Click events not working

**Cause:** Modal bounds not correctly set or event transformation wrong.

**Check:**
```cpp
// Modal bounds
LOG_DEBUG("Modal: x=%d, y=%d, w=%d, h=%d", 
          GetX(), GetY(), GetWidth(), GetHeight());

// Click coordinates
LOG_DEBUG("Click: x=%d, y=%d (local: %d, %d)",
          screen_x, screen_y, local_x, local_y);
```

#### Scrolling not working

**Cause:** ScrollView not receiving events or not set up correctly.

**Solution:**
```cpp
// Ensure ScrollView is the content
auto scroll = std::make_shared<ScrollView>();
scroll->SetChild(content_vbox);
scroll->ShowScrollbar(true);
SetContent(scroll);  // ScrollView is the root

// Check HandleScroll is forwarding
bool MyModal::HandleScroll(int x, int y, double dx, double dy) {
    return content_widget_->HandleScroll(x, y, dx, dy);
}
```

---

## Performance

### Optimization Tips

1. **Lazy content building** - Build content only when shown
2. **Cached layouts** - Calculate widget sizes once
3. **Efficient rendering** - Only redraw when content changes
4. **Memory management** - Clear large content when dismissed

```cpp
class OptimizedModal : public BaseModal {
public:
    OptimizedModal() : BaseModal(600, 400), content_built_(false) {}
    
    void Show() override {
        if (!content_built_) {
            BuildContent();  // Build once on first show
            content_built_ = true;
        }
        BaseModal::Show();
    }
    
    void OnClose() override {
        // Optional: Clear large content
        if (ShouldClearContent()) {
            ClearContent();
            content_built_ = false;
        }
    }

private:
    bool content_built_;
};
```

---

## See Also

- [Popover System](/docs/features/popovers) - Smaller context menus near widgets
- [Widget System](/docs/development/widget-system) - Widget composition and architecture
- [ScrollView](/docs/development/widgets/scrollview) - Scrollable container widget
- [Keybindings](/docs/getting-started/keybindings) - Triggering modals with shortcuts
