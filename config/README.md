# LeviathanDM Configuration

## Configuration Files

LeviathanDM looks for configuration files in the following order:

1. `~/.config/leviathan/leviathan.yaml` (user config)
2. `/etc/leviathan/leviathan.yaml` (system config)

## Configuration Structure

The configuration file uses YAML format and supports includes for modular configuration.

### Main Configuration

```yaml
# Include other config files
include:
  - libinput.yaml
  - keybindings.yaml

general:
  terminal: alacritty
  auto_launch_terminal: true
  border_width: 2
  border_color: "#ff0000"

libinput:
  mouse:
    speed: 0.5          # -1.0 to 1.0
    natural_scroll: false
    accel_profile: adaptive  # "adaptive" or "flat"
  
  touchpad:
    speed: 0.0
    natural_scroll: true
    tap_to_click: true
    tap_and_drag: true
    accel_profile: adaptive
  
  keyboard:
    repeat_rate: 25     # characters per second
    repeat_delay: 600   # milliseconds
```

### Split Configuration Example

You can split your configuration into multiple files:

**~/.config/leviathan/leviathan.yaml**:
```yaml
include:
  - libinput.yaml

general:
  terminal: alacritty
  auto_launch_terminal: true
```

**~/.config/leviathan/libinput.yaml**:
```yaml
libinput:
  mouse:
    speed: 0.7
    natural_scroll: false
```

### Configuration Options

#### General Settings

- `terminal`: Command to launch terminal (default: "alacritty")
- `auto_launch_terminal`: Launch terminal on startup (default: true)
- `border_width`: Window border width in pixels (default: 2)
- `border_color`: Border color in hex format (default: "#ff0000")

#### LibInput Settings

##### Mouse
- `speed`: Pointer speed, -1.0 (slow) to 1.0 (fast), default: 0.5
- `natural_scroll`: Reverse scroll direction (default: false)
- `accel_profile`: "adaptive" (with acceleration) or "flat" (no acceleration)

##### Touchpad
- `speed`: Touchpad speed, -1.0 to 1.0, default: 0.0
- `natural_scroll`: Reverse scroll direction (default: true)
- `tap_to_click`: Enable tap-to-click (default: true)
- `tap_and_drag`: Enable tap-and-drag (default: true)
- `accel_profile`: "adaptive" or "flat"

##### Keyboard
- `repeat_rate`: Key repeat rate in characters per second (default: 25)
- `repeat_delay`: Delay before repeat starts in milliseconds (default: 600)

## Quick Start

1. Copy the example configuration:
   ```bash
   mkdir -p ~/.config/leviathan
   cp config/leviathan.yaml ~/.config/leviathan/
   ```

2. Edit the configuration:
   ```bash
   $EDITOR ~/.config/leviathan/leviathan.yaml
   ```

3. Restart the compositor for changes to take effect

## Include Paths

Include paths can be:
- Relative to the config file directory
- Absolute paths

Example:
```yaml
include:
  - libinput.yaml                          # Relative to config directory
  - /etc/leviathan/shared.yaml             # Absolute path
  - ~/.config/leviathan/keybindings.yaml   # Home directory
```

## Notes

- Later configuration values override earlier ones
- Main config file values override included files
- Includes are processed recursively
- Circular includes are detected and prevented
