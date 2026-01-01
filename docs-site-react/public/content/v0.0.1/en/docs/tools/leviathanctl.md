# leviathanctl - IPC Control Tool

`leviathanctl` is the command-line control tool for LeviathanDM. It uses IPC (Inter-Process Communication) to interact with the running compositor, allowing you to query state, control behavior, and execute actions without restarting.

## Overview

The tool communicates with the compositor through a Unix domain socket, sending JSON commands and receiving structured responses. This enables scripting, automation, and integration with other tools.

## Installation

`leviathanctl` is built automatically when you build LeviathanDM:

```bash
mkdir build && cd build
cmake ..
make
```

The binary will be located at `build/leviathanctl`.

## Basic Usage

```bash
leviathanctl <command> [args]
```

Get help:
```bash
leviathanctl --help
leviathanctl -h
```

## Available Commands

### Connection Testing

#### `ping`
Test connection to the compositor.

```bash
leviathanctl ping
```

**Output:**
```
Pong! Compositor is running.
```

**Use Cases:**
- Check if compositor is running
- Test IPC connection in scripts
- Verify socket permissions

---

### Version Information

#### `version`
Get the compositor version.

```bash
leviathanctl version
```

**Output:**
```
LeviathanDM version 0.0.4
```

**Use Cases:**
- Check which version is running
- Verify updates were applied
- Script compatibility checking

---

### Tag Management

#### `get-tags`
List all tags with their current state.

```bash
leviathanctl get-tags
```

**Output:**
```
Tags:
  1 [ACTIVE] (2 clients)
  2 (0 clients)
  3 (1 client)
  4 (0 clients)
  5 (0 clients)
```

**Fields:**
- Tag number/name
- `[ACTIVE]` - Currently visible tag
- Client count - Number of windows on that tag

**Use Cases:**
- Monitor workspace usage
- Create workspace switcher widgets
- Display tag info in status bars

#### `get-active-tag`
Get the currently active (visible) tag.

```bash
leviathanctl get-active-tag
```

**Output:**
```
Active tag: 1
```

**Use Cases:**
- Display current workspace in custom bars
- Script conditional behavior based on workspace
- Track workspace switching for statistics

#### `set-active-tag <name>`
Switch to a different tag.

```bash
leviathanctl set-active-tag 2
leviathanctl set-active-tag dev
```

**Arguments:**
- `<name>` - Tag number or name to switch to

**Output:**
```
Switched to tag: 2
```

**Use Cases:**
- Switch workspaces from scripts
- Create custom workspace switchers
- Automate window management workflows
- Integrate with external tools (rofi, dmenu)

**Example Script:**
```bash
#!/usr/bin/env fish
# Quick workspace switcher
set tag (seq 1 5 | rofi -dmenu -p "Switch to tag:")
if test -n "$tag"
    leviathanctl set-active-tag $tag
end
```

---

### Window Management

#### `get-clients`
List all windows (clients) currently managed by the compositor.

```bash
leviathanctl get-clients
```

**Output:**
```
Clients (3):
  [1] kitty (tag: 1, floating: no)
      app_id: kitty
      title: ~/Projects/LeviathanDM — fish
      size: 1920x1080
      position: 0,0
      
  [2] firefox (tag: 1, floating: no)
      app_id: firefox
      title: LeviathanDM Documentation
      size: 1920x1080
      position: 0,0
      
  [3] Alacritty (tag: 2, floating: yes)
      app_id: Alacritty
      title: alacritty
      size: 800x600
      position: 560,240
```

**Fields:**
- Window index
- Application name
- Tag assignment
- Floating status
- `app_id` - Wayland application ID
- `title` - Window title
- `size` - Width x Height
- `position` - X, Y coordinates

**Use Cases:**
- **Debug window rules** - Check actual app_id values for pattern matching
- Monitor window layout
- Create window switcher tools
- Track application usage
- Verify decoration rules are applying correctly

**Common Debugging Pattern:**
```bash
# 1. Check what app_id your application actually uses
leviathanctl get-clients

# 2. Use the exact app_id in your window rules
# In leviathan.yaml:
window-rules:
  - name: terminal-transparent
    app_id: "kitty"  # Use exact match from get-clients
    decoration_group: transparent
```

---

### Display Management

#### `get-outputs`
List all connected displays (monitors).

```bash
leviathanctl get-outputs
```

**Output:**
```
Outputs (2):
  [1] DP-1 (primary)
      resolution: 2560x1440 @ 144Hz
      position: 0,0
      scale: 1.0
      transform: normal
      enabled: yes
      
  [2] HDMI-A-1
      resolution: 1920x1080 @ 60Hz
      position: 2560,0
      scale: 1.0
      transform: normal
      enabled: yes
```

**Fields:**
- Output name (connector)
- Primary display indicator
- Resolution and refresh rate
- Position in layout
- Scale factor
- Transform/rotation
- Enabled status

**Use Cases:**
- Monitor multi-display setup
- Verify display configuration
- Create display management tools
- Debug output issues

---

### Layout Information

#### `get-layout`
Get the current layout mode.

```bash
leviathanctl get-layout
```

**Output:**
```
Current layout: tile
```

**Possible Values:**
- `tile` - Tiled layout
- `float` - Floating layout
- `monocle` - Fullscreen/maximized
- Custom layout names

**Use Cases:**
- Display current layout in status bar
- Script different behaviors per layout
- Monitor layout changes

---

### Performance Monitoring

#### `get-plugin-stats`
Show memory usage statistics for loaded plugins.

```bash
leviathanctl get-plugin-stats
```

**Output:**
```
Plugin Statistics:

Battery Widget:
  Memory: 245 KB
  Status: Active
  Last Update: 2s ago
  
Clock Widget:
  Memory: 128 KB
  Status: Active
  Last Update: 1s ago
  
Tags Widget:
  Memory: 89 KB
  Status: Active
  Last Update: 0s ago

Total Plugin Memory: 462 KB
```

**Fields:**
- Plugin name
- Memory usage
- Active status
- Last update time

**Use Cases:**
- Monitor plugin performance
- Debug memory leaks
- Optimize plugin usage
- Profile system resources

---

### Action Execution

#### `action <name>`
Execute a named action defined in your configuration or built into the compositor.

```bash
leviathanctl action show-help
leviathanctl action reload-config
leviathanctl action screenshot
```

**Arguments:**
- `<name>` - Action name to execute

**Output:**
```
Executed action: show-help
```

**Common Actions:**
- `show-help` - Display keybindings help modal
- `reload-config` - Reload configuration without restart
- `screenshot` - Take a screenshot
- Custom actions from your config

**Use Cases:**
- Trigger actions from scripts
- Create custom keybinding tools
- Automate workflows
- Integrate with external tools

**Example - Screenshot Workflow:**
```bash
#!/usr/bin/env fish
# Take screenshot and copy to clipboard
leviathanctl action screenshot
sleep 0.5
wl-copy < ~/Pictures/screenshot-latest.png
notify-send "Screenshot copied to clipboard"
```

---

### System Control

#### `shutdown`
Gracefully shutdown the compositor.

```bash
leviathanctl shutdown
```

**Interactive Confirmation:**
```
This will gracefully shutdown the LeviathanDM compositor.
Are you sure? [y/N] y
Shutting down compositor...
```

**Output:**
```
Compositor shutdown initiated.
```

**Use Cases:**
- Clean shutdown from scripts
- Power management integration
- Session management
- Emergency recovery

**Safety:**
- Always asks for confirmation (unless scripted)
- Saves state before shutdown
- Closes applications gracefully
- Cleans up resources

**Script Usage (skip confirmation):**
```bash
echo "y" | leviathanctl shutdown
```

---

## Scripting Examples

### Workspace Switcher with rofi

```bash
#!/usr/bin/env fish
# Interactive workspace switcher

# Get all tags
set tags (leviathanctl get-tags | grep -oP '^\s*\K\d+')

# Show menu
set selected (printf "%s\n" $tags | rofi -dmenu -p "Workspace:")

# Switch to selected
if test -n "$selected"
    leviathanctl set-active-tag $selected
end
```

### Monitor System Status

```bash
#!/usr/bin/env fish
# Display system status

echo "=== LeviathanDM Status ==="
echo ""
echo "Version:"
leviathanctl version
echo ""
echo "Active Workspace:"
leviathanctl get-active-tag
echo ""
echo "Windows:"
leviathanctl get-clients | grep -c "Clients"
echo ""
echo "Memory Usage:"
leviathanctl get-plugin-stats | grep "Total"
```

### Auto-organize Windows

```bash
#!/usr/bin/env fish
# Move all terminals to workspace 1, browsers to 2

set clients (leviathanctl get-clients)

# Parse and organize
for line in $clients
    if string match -q "*kitty*" $line
        # Move to tag 1
        leviathanctl action move-window-to-tag-1
    else if string match -q "*firefox*" $line
        # Move to tag 2
        leviathanctl action move-window-to-tag-2
    end
end
```

### Display Panel Widget

```bash
#!/usr/bin/env fish
# Output current workspace for i3bar/polybar/waybar

while true
    set active (leviathanctl get-active-tag | grep -oP '\d+')
    set count (leviathanctl get-clients | grep "tag: $active" | wc -l)
    
    echo "Workspace: $active ($count windows)"
    sleep 1
end
```

---

## Error Handling

### Connection Errors

```bash
Error: Could not connect to LeviathanDM compositor
Make sure the compositor is running.
```

**Solutions:**
- Check if compositor is running: `ps aux | grep leviathan`
- Verify socket file exists: `ls /tmp/leviathan-socket`
- Check permissions: `ls -l /tmp/leviathan-socket`

### Command Errors

```bash
Error: Unknown command 'foo'
```

**Solution:** Check spelling and use `leviathanctl --help` for available commands.

### Permission Errors

If the socket has incorrect permissions:

```bash
chmod 666 /tmp/leviathan-socket
```

---

## Configuration

The IPC socket is automatically created when the compositor starts. Default location:

```
/tmp/leviathan-socket
```

### Custom Socket Path

To use a custom socket path, set the environment variable:

```bash
export LEVIATHAN_SOCKET=/run/user/1000/leviathan.sock
```

Both the compositor and `leviathanctl` respect this variable.

---

## Integration Examples

### waybar Integration

```json
{
    "custom/leviathan-workspace": {
        "exec": "leviathanctl get-active-tag | grep -oP '\\d+'",
        "interval": 1,
        "format": " {}",
        "on-click": "rofi-workspace-switcher"
    }
}
```

### i3status Integration

```python
# i3status plugin
import subprocess

def get_workspace():
    result = subprocess.run(['leviathanctl', 'get-active-tag'], 
                          capture_output=True, text=True)
    return result.stdout.strip().split()[-1]
```

---

## Troubleshooting

### leviathanctl hangs
- Check if compositor is frozen
- Verify socket is not locked
- Try `pkill -9 leviathanctl` and retry

### Commands return empty output
- Check if you have permissions to read socket
- Verify compositor version matches tool version
- Check compositor logs: `journalctl --user -u leviathan`

### Action not found
- List available actions in your config
- Check action name spelling
- Verify action is defined in `leviathan.yaml`

---

## See Also

- [Window Rules](/docs/features/window-rules) - Using get-clients for debugging
- [Configuration](/docs/getting-started/configuration) - Defining custom actions
- [IPC Protocol](/docs/development/ipc-protocol) - Low-level protocol details (future)
