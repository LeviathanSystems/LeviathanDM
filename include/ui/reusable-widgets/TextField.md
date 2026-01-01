# TextField Widget

A flexible text input widget with two visual variants: Standard and Outlined.

## Features

- **Two Variants**:
  - **Standard**: Clean, borderless input area (default)
  - **Outlined**: Input with visible border and focus indication
- **Text manipulation**: Insert, delete, cursor movement
- **Selection support**: Text selection (visual feedback implemented)
- **Keyboard navigation**: Arrow keys, Home, End
- **Callbacks**: Text change, submit (Enter), focus, blur events
- **Customizable styling**: Colors, fonts, padding, border radius
- **Cursor blinking**: Animated cursor when focused
- **Placeholder text**: Show hint when empty

## Usage

### Basic Usage

```cpp
#include "ui/reusable-widgets/TextField.hpp"

// Create standard variant (no border)
auto textField = std::make_shared<TextField>("Enter text...");

// Or create outlined variant (with border)
auto textFieldOutlined = std::make_shared<TextField>("Search...", TextField::Variant::Outlined);
```

### Styling

```cpp
// Set colors
textField->SetTextColor(1.0, 1.0, 1.0, 1.0);           // White text
textField->SetPlaceholderColor(0.6, 0.6, 0.6, 1.0);    // Gray placeholder
textField->SetBackgroundColor(0.15, 0.15, 0.15, 1.0);  // Dark background
textField->SetBorderColor(0.4, 0.4, 0.4, 1.0);         // Gray border
textField->SetFocusBorderColor(0.3, 0.5, 0.8, 1.0);    // Blue when focused

// Set dimensions
textField->SetMinWidth(200);
textField->SetMaxWidth(400);
textField->SetPadding(8);
textField->SetBorderWidth(1);
textField->SetBorderRadius(4);

// Set font
textField->SetFontSize(12);
textField->SetFontFamily("JetBrainsMono Nerd Font");
```

### Callbacks

```cpp
// Text changed callback
textField->SetOnTextChanged([](const std::string& text) {
    LOG_DEBUG_FMT("Text changed: {}", text);
});

// Submit callback (Enter key pressed)
textField->SetOnSubmit([](const std::string& text) {
    LOG_INFO_FMT("Submitted: {}", text);
});

// Focus/blur callbacks
textField->SetOnFocus([]() {
    LOG_DEBUG("TextField focused");
});

textField->SetOnBlur([]() {
    LOG_DEBUG("TextField blurred");
});
```

### Text Manipulation

```cpp
// Set text programmatically
textField->SetText("Hello World");

// Get current text
std::string text = textField->GetText();

// Change placeholder
textField->SetPlaceholder("Type here...");

// Focus/blur programmatically
textField->Focus();
textField->Blur();

// Check if focused
if (textField->IsFocused()) {
    // ...
}
```

### Integration Example

```cpp
// In your UI layout
auto container = std::make_shared<HBox>();

// Add label
auto label = std::make_shared<Label>("Search:");
container->AddChild(label);

// Add text field
auto searchField = std::make_shared<TextField>("Type to search...", TextField::Variant::Outlined);
searchField->SetMinWidth(300);
searchField->SetOnTextChanged([this](const std::string& query) {
    this->OnSearchQueryChanged(query);
});
container->AddChild(searchField);

// Add search button
auto searchBtn = std::make_shared<Button>("Search");
searchBtn->SetOnClick([searchField]() {
    std::string query = searchField->GetText();
    // Perform search...
});
container->AddChild(searchBtn);
```

### Cursor Blinking

The cursor blinks automatically when the text field is focused. To enable this, call `UpdateCursorBlink()` periodically (e.g., in your render loop):

```cpp
// In your render/update loop
uint32_t current_time = GetCurrentTimeMs();  // Your time function
textField->UpdateCursorBlink(current_time);
```

## Keyboard Controls

- **Printable characters**: Insert at cursor position
- **Backspace**: Delete character before cursor
- **Delete**: Delete character after cursor
- **Left/Right Arrow**: Move cursor
- **Home**: Move cursor to start
- **End**: Move cursor to end
- **Ctrl+A**: Select all text
- **Ctrl+C**: Copy selection (TODO: requires clipboard integration)
- **Ctrl+V**: Paste (TODO: requires clipboard integration)
- **Enter**: Trigger submit callback

## Visual Differences

### Standard Variant
- No visible border
- Simple, clean appearance
- Background color only
- Perfect for inline editing

### Outlined Variant
- Visible border around input
- Border changes color when focused
- Clear visual boundaries
- Better for forms and structured input

## Notes

- Clipboard operations (copy/paste) require Wayland clipboard integration (TODO)
- Text selection is visually implemented but clipboard copy needs Wayland integration
- The widget is thread-safe with mutex protection
- Cursor position is maintained across focus changes
- Empty text shows placeholder text in gray

## Default Colors

- **Text**: White (1.0, 1.0, 1.0, 1.0)
- **Placeholder**: Gray (0.6, 0.6, 0.6, 1.0)
- **Background**: Dark gray (0.15, 0.15, 0.15, 1.0)
- **Border**: Gray (0.4, 0.4, 0.4, 1.0)
- **Focus Border**: Blue (0.3, 0.5, 0.8, 1.0)
- **Cursor**: White (1.0, 1.0, 1.0, 1.0)
- **Selection**: Semi-transparent blue (0.3, 0.5, 0.8, 0.3)
