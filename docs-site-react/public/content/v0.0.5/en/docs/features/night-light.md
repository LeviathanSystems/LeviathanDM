---
weight: 15
title: "Night Light"
---

# Night Light

**New in v0.0.5**

The Night Light feature automatically reduces blue light during evening and night hours by applying a warm color temperature overlay to your screen. This helps reduce eye strain and improve sleep quality during nighttime computer use.

## Overview

Night Light works by:
- **Time-based activation**: Automatically turns on/off based on your configured schedule
- **Temperature adjustment**: Applies warm color tones (1000K-6500K range)
- **Smooth transitions**: Gradually fades in/out over a configurable period
- **Customizable intensity**: Adjust the strength of the effect from subtle to strong

## Configuration

Night Light is configured in your `~/.config/leviathan/leviathan.yaml` file under the `night_light` section:

```yaml
night_light:
  enabled: true              # Enable/disable the feature
  start_time: "20:00"        # When to activate (24-hour format)
  end_time: "06:00"          # When to deactivate
  temperature: 4500          # Color temperature in Kelvin
  strength: 0.50             # Effect intensity (0.0-1.0)
  transition_duration: 1800  # Fade time in seconds (30 min)
  smooth_transition: true    # Enable gradual strength changes
```

### Configuration Options

#### `enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Master switch to enable or disable the night light feature

#### `start_time`
- **Type**: String (HH:MM format)
- **Default**: `"20:00"`
- **Description**: Time when night light activates (24-hour format)
- **Example**: `"18:30"` for 6:30 PM

#### `end_time`
- **Type**: String (HH:MM format)
- **Default**: `"06:00"`
- **Description**: Time when night light deactivates
- **Note**: Can be earlier than start_time for overnight schedules (e.g., 20:00-06:00)

#### `temperature`
- **Type**: Integer
- **Default**: `3400`
- **Range**: 1000-6500 Kelvin
- **Description**: Color temperature of the night light effect
- **Common values**:
  - `6500K` - Neutral/Daylight (no effect)
  - `5000K` - Cool white
  - `4500K` - Late afternoon (recommended)
  - `3400K` - Sunset/Warm
  - `2700K` - Incandescent bulb
  - `1850K` - Candlelight (very warm)

#### `strength`
- **Type**: Float
- **Default**: `0.85`
- **Range**: 0.0-1.0
- **Description**: Intensity of the color temperature effect
- **Recommendations**:
  - `0.3-0.5` - Subtle effect
  - `0.5-0.7` - Moderate (recommended)
  - `0.7-0.9` - Strong effect

#### `transition_duration`
- **Type**: Integer
- **Default**: `1800` (30 minutes)
- **Unit**: Seconds
- **Description**: How long the fade-in/fade-out transition takes
- **Examples**:
  - `900` - 15 minutes
  - `1800` - 30 minutes (recommended)
  - `3600` - 1 hour

#### `smooth_transition`
- **Type**: Boolean
- **Default**: `true`
- **Description**: Enable gradual strength changes during transitions
- **Note**: When `false`, night light turns on/off instantly at the scheduled times

## Usage Examples

### Standard Evening Protection
Activate from 8 PM to 6 AM with moderate warmth:

```yaml
night_light:
  enabled: true
  start_time: "20:00"
  end_time: "06:00"
  temperature: 4500
  strength: 0.50
  transition_duration: 1800
  smooth_transition: true
```

### Late Night Work
Stronger effect for late-night coding sessions:

```yaml
night_light:
  enabled: true
  start_time: "22:00"
  end_time: "07:00"
  temperature: 3400
  strength: 0.75
  transition_duration: 900
  smooth_transition: true
```

### Subtle Protection
Light effect that doesn't dramatically change colors:

```yaml
night_light:
  enabled: true
  start_time: "19:00"
  end_time: "06:00"
  temperature: 5000
  strength: 0.35
  transition_duration: 1800
  smooth_transition: true
```

### Instant On/Off
No transition effects:

```yaml
night_light:
  enabled: true
  start_time: "20:00"
  end_time: "06:00"
  temperature: 4500
  strength: 0.50
  transition_duration: 0
  smooth_transition: false
```

## Tips & Recommendations

### Finding Your Ideal Settings

1. **Start Conservative**: Begin with `temperature: 4500` and `strength: 0.50`
2. **Test in Evening**: Wait for your configured start time to see the effect
3. **Adjust Gradually**: Increase strength by 0.1 increments if too subtle
4. **Lower Temperature**: If color is too orange, try 4500K-5000K range
5. **Test Before Bed**: The effect should be noticeable but not jarring

### Color Temperature Guide

The color temperature scale represents the color of light sources:

| Kelvin | Color Appearance | Use Case |
|--------|------------------|----------|
| 6500K | Daylight/Neutral | No effect (disabled) |
| 5500K | Cool White | Very subtle |
| 5000K | White | Light protection |
| 4500K | Late Afternoon | **Recommended start** |
| 4000K | Warm White | Moderate protection |
| 3400K | Sunset | Strong effect |
| 2700K | Incandescent | Very warm |
| 2000K | Candlelight | Extreme (may distort colors) |

### Common Issues

**Too Orange?**
- Increase `temperature` to 4500-5000K
- Reduce `strength` to 0.4-0.5

**Not Noticeable?**
- Lower `temperature` to 3400-4000K
- Increase `strength` to 0.6-0.8

**Colors Look Wrong?**
- Use higher temperature (4500K+) for color-critical work
- Consider disabling during photo/video editing

**Sudden Changes?**
- Increase `transition_duration` to 1800+ seconds
- Ensure `smooth_transition: true`

## How It Works

Night Light uses a scientifically-based color temperature algorithm to shift the screen's color palette:

1. **Temperature to RGB Conversion**: Uses the Tanner Helland algorithm to convert Kelvin temperatures to RGB values
2. **Color Tinting**: Further reduces blue and green channels for warmer appearance
3. **Alpha Blending**: Applies the colored overlay with configurable opacity (strength)
4. **Scene Layer**: Renders on a dedicated top-most layer that doesn't interfere with windows

The implementation ensures:
- **Minimal Performance Impact**: Single overlay rectangle per screen
- **No Color Banding**: Smooth gradients with proper alpha blending
- **Per-Output Support**: Different settings can be applied to multiple monitors

## Reloading Configuration

After editing your configuration file, reload it to apply changes:

```bash
# Using leviathanctl
leviathanctl reload-config

# Or use the keybinding (default: Super+Shift+R)
```

The night light will immediately adjust to your new settings without restarting the compositor.

## Related Features

- **[Status Bar](status-bar)**: Can display night light status (with plugin)
- **[Configuration](../configuration)**: General configuration file guide
- **[Keybindings](../configuration/keybindings)**: Add custom shortcuts to toggle night light

## Health Benefits

Research suggests that reducing blue light exposure in the evening may:
- Improve sleep quality and duration
- Reduce eye strain during nighttime screen use
- Help maintain natural circadian rhythms
- Reduce headaches from prolonged screen time

**Note**: Night Light is a display filter and not a substitute for proper sleep hygiene. Consider also reducing screen brightness and taking regular breaks.
