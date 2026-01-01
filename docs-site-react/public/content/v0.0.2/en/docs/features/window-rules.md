# Window Rules

**Available since:** v0.0.2

Window rules allow you to automatically apply window decorations and behaviors to windows based on pattern matching. This gives you fine-grained control over how different applications look and behave.

## Overview

Window rules match windows based on:
- **app_id** - Wayland application identifier
- **title** - Window title text
- **floating** - Whether window is floating

When a window matches a rule, it automatically applies:
- Window decorations (borders, shadows, opacity)
- Layout behaviors
- Other window properties

**Common Use Cases:**
- Transparent terminals
- Borderless gaming windows
- Floating utility windows
- Application-specific styling
- Different styles for different window types

---

## Basic Configuration

Window rules are defined in your `leviathan.yaml`:

```yaml
window-rules:
  - name: rule-name
    app_id: "pattern"
    decoration_group: decoration-name
```

**Example:**

```yaml
window-decorations:
  - name: transparent
    border_width: 2
    border_color_focused: "#5E81AC"
    border_color_unfocused: "#3B4252"
    opacity: 0.9
    opacity_inactive: 0.8
    border_radius: 8
    enable_shadows: true

window-rules:
  - name: transparent-terminal
    app_id: "kitty"
    decoration_group: transparent
```

This applies the `transparent` decoration to all kitty terminal windows.

---

## Pattern Matching

### Exact Match

Match a specific app_id:

```yaml
window-rules:
  - name: firefox-rule
    app_id: "firefox"
    decoration_group: browser-style
```

Matches only windows with `app_id = "firefox"`.

### Wildcard Match

Use `*` to match any sequence of characters:

```yaml
window-rules:
  # Match all terminals
  - name: all-terminals
    app_id: "*term*"
    decoration_group: terminal-style

  # Match all Electron apps
  - name: electron-apps
    app_id: "*electron*"
    decoration_group: electron-style

  # Match everything
  - name: default-all
    app_id: "*"
    decoration_group: default-style
```

**Wildcard Behavior:**
- `*` matches zero or more characters
- `*term*` matches "kitty", "alacritty", "terminology", etc.
- `electron*` matches "electron-app", "electron"
- `*` matches everything

### Title Match

Match based on window title:

```yaml
window-rules:
  # Match by title
  - name: vim-windows
    title: "*vim*"
    decoration_group: editor-style

  # Match specific title
  - name: htop-window
    title: "htop"
    decoration_group: monitor-style
```

### Combined Match

Match based on multiple criteria (all must match):

```yaml
window-rules:
  # Floating Firefox windows (picture-in-picture)
  - name: firefox-floating
    app_id: "firefox"
    floating: true
    decoration_group: floating-style

  # Non-floating terminal
  - name: terminal-tiled
    app_id: "kitty"
    floating: false
    decoration_group: tiled-style
```

---

## Rule Priority

Rules are evaluated in order from **top to bottom**. The **first matching rule** is applied.

```yaml
window-rules:
  # Specific rule (checked first)
  - name: kitty-transparent
    app_id: "kitty"
    decoration_group: transparent

  # General rule (checked second)
  - name: all-terminals
    app_id: "*term*"
    decoration_group: default-terminal

  # Fallback (checked last)
  - name: default-all
    app_id: "*"
    decoration_group: default
```

**Matching order:**
1. `kitty` → Uses `transparent`
2. `alacritty` → Matches `*term*` → Uses `default-terminal`
3. `firefox` → Matches `*` → Uses `default`

**Best Practice:** Put more specific rules before general rules.

---

## Finding app_id

### Method 1: Use leviathanctl

The easiest way to find app_id:

```bash
leviathanctl get-clients
```

**Output:**
```
Clients (3):
  [1] kitty (tag: 1, floating: no)
      app_id: kitty
      title: ~/Projects — fish
      size: 1920x1080
      position: 0,0
```

The `app_id:` field shows the exact string to use in your rules.

### Method 2: Compositor Logs

When a window is created, LeviathanDM logs the app_id:

```bash
# Watch logs in real time
journalctl --user -u leviathan -f

# Look for lines like:
# New window: app_id=firefox, title=Mozilla Firefox
```

### Method 3: Application Documentation

Some common app_ids:
- **Terminals:** `kitty`, `Alacritty`, `foot`, `wezterm`
- **Browsers:** `firefox`, `chromium`, `google-chrome`
- **Editors:** `code` (VS Code), `sublime_text`
- **Tools:** `thunar`, `nautilus`, `dolphin` (file managers)

---

## Common Patterns

### Transparent Terminals

```yaml
window-decorations:
  - name: transparent-term
    border_width: 2
    border_color_focused: "#88C0D0"
    border_color_unfocused: "#4C566A"
    opacity: 0.9
    opacity_inactive: 0.85
    border_radius: 6
    enable_shadows: true
    shadow_size: 15
    shadow_color: "#000000"
    shadow_opacity: 0.5

window-rules:
  - name: kitty-transparent
    app_id: "kitty"
    decoration_group: transparent-term
    
  - name: alacritty-transparent
    app_id: "Alacritty"
    decoration_group: transparent-term
```

### Borderless Gaming Windows

```yaml
window-decorations:
  - name: borderless-game
    border_width: 0
    opacity: 1.0
    opacity_inactive: 1.0
    border_radius: 0
    enable_shadows: false

window-rules:
  - name: steam-games
    app_id: "*steam*"
    decoration_group: borderless-game
    
  - name: wine-games
    app_id: "*wine*"
    decoration_group: borderless-game
```

### Floating Windows with Fancy Styling

```yaml
window-decorations:
  - name: fancy-floating
    border_width: 3
    border_color_focused: "#EBCB8B"
    border_color_unfocused: "#5E81AC"
    opacity: 1.0
    opacity_inactive: 0.95
    border_radius: 12
    enable_shadows: true
    shadow_size: 20
    shadow_color: "#000000"
    shadow_opacity: 0.7
    shadow_offset_x: 0
    shadow_offset_y: 4

window-rules:
  - name: floating-windows
    floating: true
    decoration_group: fancy-floating
```

### Browser Styling

```yaml
window-decorations:
  - name: browser-style
    border_width: 1
    border_color_focused: "#8FBCBB"
    border_color_unfocused: "#4C566A"
    opacity: 1.0
    opacity_inactive: 0.98
    border_radius: 0
    enable_shadows: true
    shadow_size: 10

window-rules:
  - name: firefox
    app_id: "firefox"
    decoration_group: browser-style
    
  - name: chromium
    app_id: "chromium"
    decoration_group: browser-style
    
  - name: chrome
    app_id: "google-chrome"
    decoration_group: browser-style
```

### Application Categories

```yaml
window-decorations:
  - name: dev-tools
    border_color_focused: "#A3BE8C"  # Green
    # ...
    
  - name: media-players
    border_color_focused: "#BF616A"  # Red
    # ...
    
  - name: communication
    border_color_focused: "#88C0D0"  # Blue
    # ...

window-rules:
  # Development tools
  - name: vscode
    app_id: "code"
    decoration_group: dev-tools
    
  - name: terminals
    app_id: "*term*"
    decoration_group: dev-tools
  
  # Media players
  - name: mpv
    app_id: "mpv"
    decoration_group: media-players
    
  - name: spotify
    app_id: "spotify"
    decoration_group: media-players
  
  # Communication
  - name: discord
    app_id: "*discord*"
    decoration_group: communication
    
  - name: slack
    app_id: "slack"
    decoration_group: communication
```

---

## Decoration Properties Reference

All available decoration options:

```yaml
window-decorations:
  - name: example
    # Border
    border_width: 2              # Pixels (default: 2)
    border_color_focused: "#5E81AC"
    border_color_unfocused: "#3B4252"
    border_radius: 8             # Pixels (default: 0)
    
    # Opacity
    opacity: 1.0                 # 0.0 - 1.0 (default: 1.0)
    opacity_inactive: 0.95       # 0.0 - 1.0 (default: 1.0)
    
    # Shadows
    enable_shadows: true         # Boolean (default: false)
    shadow_size: 15              # Pixels (default: 10)
    shadow_color: "#000000"      # Hex color (default: black)
    shadow_opacity: 0.5          # 0.0 - 1.0 (default: 0.6)
    shadow_offset_x: 0           # Pixels (default: 0)
    shadow_offset_y: 2           # Pixels (default: 0)
    
    # Dimming
    dim_inactive: true           # Boolean (default: false)
    dim_amount: 0.2              # 0.0 - 1.0 (default: 0.3)
```

### Border Properties

- **border_width:** Line thickness around window
- **border_color_focused:** Color when window has focus
- **border_color_unfocused:** Color when window doesn't have focus
- **border_radius:** Rounded corners (0 = square)

### Opacity Properties

- **opacity:** Transparency of focused window (1.0 = opaque, 0.0 = invisible)
- **opacity_inactive:** Transparency of unfocused windows

**Use Cases:**
- Transparent terminals: `opacity: 0.9`
- Dim inactive windows: `opacity_inactive: 0.85`
- See-through overlays: `opacity: 0.7`

### Shadow Properties

- **enable_shadows:** Turn shadows on/off
- **shadow_size:** How far shadow extends
- **shadow_color:** Shadow color (usually black)
- **shadow_opacity:** Shadow transparency
- **shadow_offset_x/y:** Move shadow position

**Tips:**
- Larger shadows for floating windows
- Small shadows for tiled windows
- Offset Y for depth effect: `shadow_offset_y: 4`

### Dimming Properties

- **dim_inactive:** Darken unfocused windows
- **dim_amount:** How much to dim (0.0 = no dim, 1.0 = fully dark)

**Use Case:** Make focused window more obvious

---

## Multiple Decoration Groups

Create different decoration sets for different scenarios:

```yaml
window-decorations:
  # Minimal style
  - name: minimal
    border_width: 1
    border_color_focused: "#5E81AC"
    border_color_unfocused: "#3B4252"
    opacity: 1.0
    opacity_inactive: 1.0
    border_radius: 0
    enable_shadows: false

  # Fancy style
  - name: fancy
    border_width: 3
    border_color_focused: "#EBCB8B"
    border_color_unfocused: "#5E81AC"
    opacity: 1.0
    opacity_inactive: 0.95
    border_radius: 12
    enable_shadows: true
    shadow_size: 20

  # Transparent style
  - name: transparent
    border_width: 2
    border_color_focused: "#88C0D0"
    border_color_unfocused: "#4C566A"
    opacity: 0.9
    opacity_inactive: 0.85
    border_radius: 6
    enable_shadows: true

window-rules:
  # Tiled windows: minimal
  - name: tiled-terminals
    app_id: "*term*"
    floating: false
    decoration_group: minimal

  # Floating windows: fancy
  - name: floating-windows
    floating: true
    decoration_group: fancy

  # Specific apps: transparent
  - name: transparent-kitty
    app_id: "kitty"
    decoration_group: transparent
```

---

## Debugging Rules

### Check if Rules Apply

Use `leviathanctl get-clients` to verify:

```bash
$ leviathanctl get-clients
Clients (1):
  [1] kitty (tag: 1, floating: no)
      app_id: kitty
      title: ~/Projects — fish
      decorations: transparent-term  ← Shows which rule matched
```

### Common Issues

#### Rule not matching

**Symptoms:** Window doesn't get expected decoration

**Debug steps:**
1. Check app_id: `leviathanctl get-clients`
2. Verify app_id in rule matches exactly
3. Check rule order (specific before general)
4. Try wildcard: `app_id: "*kitty*"`

**Example:**
```yaml
# Wrong: app_id doesn't match
- name: terminal
  app_id: "Alacritty"  # Capital A
  decoration_group: term

# Right: Check exact app_id
$ leviathanctl get-clients
app_id: alacritty  # lowercase a

# Fix:
- name: terminal
  app_id: "alacritty"  # Match exactly
  decoration_group: term

# Or use wildcard:
- name: terminal
  app_id: "*alacritty*"  # Case-insensitive-ish
  decoration_group: term
```

#### Wrong rule applies

**Cause:** More general rule matched first

**Solution:** Reorder rules (specific first):

```yaml
# Before (wrong order):
window-rules:
  - name: all-apps        # Matches first!
    app_id: "*"
    decoration_group: default
    
  - name: kitty           # Never reached
    app_id: "kitty"
    decoration_group: transparent

# After (correct order):
window-rules:
  - name: kitty           # Checked first
    app_id: "kitty"
    decoration_group: transparent
    
  - name: all-apps        # Fallback
    app_id: "*"
    decoration_group: default
```

#### Decoration doesn't exist

**Symptoms:** Warning in logs, default decorations applied

**Check:**
```bash
journalctl --user -u leviathan | grep decoration
# Warning: decoration group 'missing-name' not found
```

**Solution:** Verify decoration name matches:

```yaml
window-decorations:
  - name: my-decoration  # Name here

window-rules:
  - name: my-rule
    decoration_group: my-decoration  # Must match exactly
```

---

## Testing Rules

### Reload Configuration

After editing rules:

```bash
# Method 1: Send reload signal
kill -HUP $(pidof leviathan)

# Method 2: Restart compositor
leviathan --replace

# Method 3: Use leviathanctl (if action defined)
leviathanctl action reload-config
```

### Test with Different Windows

1. **Open test window:**
   ```bash
   kitty  # Or any app you're testing
   ```

2. **Check app_id:**
   ```bash
   leviathanctl get-clients
   ```

3. **Verify decoration:**
   - Visual check: borders, transparency
   - Log check: `journalctl -u leviathan | grep decoration`

4. **Adjust rule and reload**

---

## Advanced Examples

### Per-Workspace Styling (Future)

Currently not supported, but planned:

```yaml
# Future feature
window-rules:
  - name: workspace-1-style
    tag: 1
    decoration_group: dev-style
    
  - name: workspace-2-style
    tag: 2
    decoration_group: browser-style
```

### Dynamic Rules (Future)

Rule engine improvements planned:

```yaml
# Future: More complex matching
window-rules:
  - name: complex-rule
    match:
      app_id: "firefox"
      title: "*YouTube*"
      fullscreen: true
    decoration_group: media-fullscreen
```

---

## Performance Notes

### Rule Evaluation

- Rules evaluated once when window created
- Fast pattern matching (O(n) where n = number of rules)
- Negligible performance impact

### Best Practices

1. **Keep rules simple** - Exact matches are fastest
2. **Minimize rules** - Don't create redundant rules
3. **Order matters** - Put common matches early
4. **Use wildcards wisely** - Too many can slow matching

```yaml
# Good: Specific, ordered by frequency
window-rules:
  - name: kitty          # Most common
    app_id: "kitty"
    decoration_group: terminal
    
  - name: firefox        # Second most common
    app_id: "firefox"
    decoration_group: browser
    
  - name: others         # Everything else
    app_id: "*"
    decoration_group: default

# Bad: Too many specific rules, no clear order
window-rules:
  - name: rare-app-1
    app_id: "some-rare-app"
    # ...
  # 50 more specific rules...
  - name: common-app
    app_id: "firefox"    # Common but last!
    # ...
```

---

## Migration from 0.0.1

### Old System (0.0.1)

```yaml
general:
  border_width: 2
  border_color_focused: "#5E81AC"
  border_color_unfocused: "#3B4252"
```

All windows had the same style.

### New System (0.0.2+)

```yaml
window-decorations:
  - name: default
    border_width: 2
    border_color_focused: "#5E81AC"
    border_color_unfocused: "#3B4252"
    opacity: 1.0
    opacity_inactive: 1.0
    border_radius: 0
    enable_shadows: false

window-rules:
  - name: default-all
    app_id: "*"
    decoration_group: default
```

Now you can have different styles per application.

---

## See Also

- [Window Decorations](/docs/features/window-decorations) - Decoration system overview
- [leviathanctl](/docs/tools/leviathanctl) - Using `get-clients` to debug rules
- [Configuration](/docs/getting-started/configuration) - Complete config file reference
