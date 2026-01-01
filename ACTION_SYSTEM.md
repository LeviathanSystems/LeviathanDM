# Action-Based Keybinding System

LeviathanDM now features a powerful action-based keybinding system that decouples keyboard shortcuts from their implementations, making keybindings reusable across different interfaces (keyboard, menubar, IPC, etc.).

## Architecture

### Actions (`include/Actions.hpp`)

Actions are named operations that can be executed from any interface:

```cpp
struct Action {
    std::string name;           // e.g., "open-terminal"
    std::string description;    // Human-readable description
    ActionType type;            // Built-in action type
    std::vector<ActionParam> params;  // Parameters for the action
};
```

### Action Registry

The `ActionRegistry` manages all available actions and handles execution:

- **Built-in Actions**: Window management, tag switching, compositor control
- **Custom Actions**: Spawn commands with parameters (e.g., "open-terminal" → "alacritty")
- **Extensible**: Can add custom actions programmatically

### Key Bindings

Key bindings now reference actions by name instead of containing inline lambdas:

```cpp
struct KeyBinding {
    uint32_t modifiers;
    xkb_keysym_t keysym;
    std::string action_name;  // References an action
};
```

## Available Actions

### Window Management
- `close-window` - Close the focused window
- `focus-next` - Focus the next window
- `focus-prev` - Focus the previous window
- `swap-next` - Swap focused window with next
- `swap-prev` - Swap focused window with previous
- `toggle-floating` - Toggle floating mode for focused window
- `toggle-fullscreen` - Toggle fullscreen for focused window

### Tag/Workspace Management  
- `switch-to-tag-N` - Switch to tag N (1-9)
- `move-to-tag-N` - Move focused window to tag N (1-9)

### Application Launching
- `open-terminal` - Open a new terminal (default: alacritty)
- `open-browser` - Open web browser (default: firefox)

### Compositor Control
- `toggle-menubar` - Toggle the application menubar
- `reload-config` - Reload configuration file (TODO)
- `shutdown` - Gracefully shutdown the compositor

## Default Key Bindings

| Key Combination | Action | Description |
|-----------------|--------|-------------|
| `Super+Return` | `open-terminal` | Open terminal |
| `Super+Shift+C` | `close-window` | Close window |
| `Super+Shift+Q` | `shutdown` | Graceful shutdown |
| `Super+J` | `focus-next` | Focus next window |
| `Super+K` | `focus-prev` | Focus previous window |
| `Super+Shift+J` | `swap-next` | Swap with next |
| `Super+Shift+K` | `swap-prev` | Swap with previous |
| `Super+F` | `toggle-floating` | Toggle floating |
| `Super+Shift+F` | `toggle-fullscreen` | Toggle fullscreen |
| `Super+1-9` | `switch-to-tag-N` | Switch to tag |
| `Super+Shift+1-9` | `move-to-tag-N` | Move window to tag |
| `Super+P` | `toggle-menubar` | Show menubar |

## Usage Examples

### From Code

Execute an action programmatically:

```cpp
// Get action registry from keybindings
auto* registry = keybindings->GetActionRegistry();

// Execute an action
registry->ExecuteAction("open-terminal");
```

### Register Custom Action

Add a new spawn command:

```cpp
registry->RegisterAction({
    .name = "open-file-manager",
    .description = "Open file manager",
    .type = ActionType::SPAWN,
    .params = {{"command", "thunar"}}
});
```

### Custom Action with Function

```cpp
registry->RegisterAction({
    .name = "toggle-notifications",
    .description = "Toggle notification visibility",
    .type = ActionType::CUSTOM,
    .params = {},
    .custom_function = []() {
        // Custom implementation
        NotificationManager::Toggle();
    }
});
```

### From MenuBar

The menubar can call actions directly:

```cpp
if (item.name == "Terminal") {
    server_->GetKeyBindings()->GetActionRegistry()->ExecuteAction("open-terminal");
}
```

### From IPC

Actions can be exposed via IPC:

```json
{
  "command": "execute_action",
  "action": "open-terminal"
}
```

## Configuration (Future)

The goal is to make keybindings configurable via YAML:

```yaml
# config/leviathan.yaml

keybindings:
  - keys: ["Super", "Return"]
    action: "open-terminal"
  
  - keys: ["Super", "B"]
    action: "open-browser"
  
  - keys: ["Super", "E"]
    action: "open-file-manager"

# Define custom spawn actions
actions:
  open-file-manager:
    type: spawn
    command: "thunar"
  
  open-editor:
    type: spawn
    command: "code"
```

## Benefits

1. **Reusability**: Actions can be triggered from keyboard, menubar, IPC, or scripts
2. **Flexibility**: Easy to add/modify actions without changing keybinding code
3. **Discoverability**: Actions have names and descriptions for UI display
4. **Configurability**: Paves the way for user-configurable keybindings
5. **Maintainability**: Single source of truth for all compositor actions
6. **Extensibility**: Custom actions can be added at runtime

## Implementation Details

### File Structure

```
include/
  Actions.hpp              # Action types and ActionRegistry
  KeyBindings.hpp          # KeyBindings class

src/
  Actions.cpp              # ActionRegistry implementation
  KeyBindings.cpp          # KeyBindings implementation
```

### Action Execution Flow

1. User presses key combination (e.g., `Super+Return`)
2. `KeyBindings::HandleKeyPress()` matches the key to a binding
3. Binding references action by name (e.g., "open-terminal")
4. `ActionRegistry::ExecuteAction()` looks up and executes the action
5. Action parameters are applied (e.g., command="alacritty")
6. Action is executed via the appropriate handler

### Parameter System

Actions can have parameters that customize their behavior:

```cpp
// SPAWN action with command parameter
{
    .type = ActionType::SPAWN,
    .params = {{"command", "firefox --new-window"}}
}

// SWITCH_TO_TAG action with tag_index parameter
{
    .type = ActionType::SWITCH_TO_TAG,
    .params = {{"tag_index", "2"}}
}
```

## Next Steps

1. **Config Parser Integration**: Load keybindings and actions from YAML
2. **IPC Action Execution**: Add IPC command to execute actions
3. **MenuBar Integration**: Use actions for menu items
4. **Action History**: Track recently used actions
5. **Action Chaining**: Support sequences of actions
6. **Conditional Actions**: Actions that depend on state

## Migration from Old System

The old system used inline lambdas:

```cpp
// OLD: Inline lambda
bindings_.push_back({mod, XKB_KEY_Return, [this]() {
    system("alacritty &");
}});
```

New system uses action names:

```cpp
// NEW: Action reference
AddBinding(mod, XKB_KEY_Return, "open-terminal");
```

Benefits:
- Actions can be called from anywhere, not just keyboards
- Actions have metadata (name, description)
- Actions can be configured externally
- Actions can be listed/discovered at runtime

## Troubleshooting

### Action Not Found

```
[ERROR] Action not found: my-custom-action
```

**Solution**: Register the action before using it:
```cpp
registry->RegisterAction({...});
```

### Failed to Execute Action

```
[ERROR] Failed to execute action: open-terminal
```

**Check**:
1. Action is registered
2. Parameters are correct
3. Server pointer is valid
4. Required resources are available

## Examples

### Create a Custom Launcher

```cpp
// Register multiple launcher actions
for (const auto& [name, cmd] : custom_launchers) {
    registry->RegisterAction({
        .name = name,
        .description = "Launch " + name,
        .type = ActionType::SPAWN,
        .params = {{"command", cmd}}
    });
}

// Bind to keys
AddBinding(MOD_SUPER, XKB_KEY_e, "open-editor");
AddBinding(MOD_SUPER, XKB_KEY_m, "open-music");
AddBinding(MOD_SUPER, XKB_KEY_v, "open-video");
```

### Screen-Specific Actions

```cpp
registry->RegisterAction({
    .name = "focus-left-monitor",
    .description = "Focus left monitor",
    .type = ActionType::CUSTOM,
    .custom_function = [this]() {
        auto screens = server_->GetScreens();
        if (screens.size() > 1) {
            // Focus logic
        }
    }
});
```

This action-based system provides a solid foundation for a flexible, user-friendly keybinding configuration system!
