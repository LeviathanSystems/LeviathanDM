---
title: "Wallpapers"
weight: 50
bookToc: true
version: "v0.1.0"
description: "Configure static wallpapers and rotating slideshows for your displays"
---

# Wallpapers

{{< version-banner version="v0.1.0" >}}

{{< version-warning type="new" version="v0.1.0" >}}
The wallpaper system was introduced in v0.1.0 with support for image rotation and per-monitor configuration.
{{< /version-warning >}}

LeviathanDM includes a built-in wallpaper system that supports static images and rotating slideshows. Wallpapers are configured per-monitor and support multiple image formats.

## Features

- **Multiple Format Support**: PNG, JPEG, BMP, WebP
- **Automatic Scaling**: Images are scaled to cover the entire output
- **Rotation Support**: Configure automatic wallpaper rotation
- **Per-Monitor Configuration**: Each monitor can have its own wallpaper
- **Folder Scanning**: Automatically load all images from a directory

## Configuration

Wallpapers are configured in the `wallpapers` section of your config file:

```yaml
wallpapers:
  # Single static wallpaper
  - name: "casual"
    wallpaper: "~/Pictures/wallpaper.png"
    change_every_seconds: 0  # 0 means no rotation
  
  # Rotating wallpapers from a list
  - name: "work"
    wallpaper:
      - "~/Pictures/work1.png"
      - "~/Pictures/work2.jpg"
      - "~/Pictures/work3.png"
    change_every_seconds: 600  # Rotate every 10 minutes
  
  # Wallpapers from a folder
  - name: "nature"
    wallpaper: "~/Pictures/Nature/"  # Folder with trailing slash
    change_every_seconds: 300  # Rotate every 5 minutes
```

### Assigning to Monitors

Reference wallpaper configurations in your monitor groups:

```yaml
monitor-groups:
  - name: "Dual Monitor Setup"
    monitors:
      - display: "eDP-1"
        wallpaper: "casual"  # Reference wallpaper config name
        position: [0, 0]
        status-bars: ["laptop-bar"]
      
      - display: "HDMI-A-1"
        wallpaper: "work"  # Different wallpaper for external monitor
        position: [1920, 0]
        status-bars: ["main-bar"]
```

## Configuration Options

| Option | Type | Description |
|--------|------|-------------|
| `name` | string | Unique identifier for this wallpaper configuration |
| `wallpaper` | string or array | Path to image file, folder, or list of images |
| `change_every_seconds` | integer | Rotation interval in seconds (0 = no rotation) |

### Path Formats

- **Single file**: `~/Pictures/wallpaper.png`
- **Multiple files**: Array of paths
- **Folder**: `~/Pictures/Wallpapers/` (with trailing slash)

{{< hint info >}}
**Tip:** Use `~` in paths to reference your home directory. Folder paths will automatically scan for `.png`, `.jpg`, `.jpeg`, `.bmp`, and `.webp` files.
{{< /hint >}}

## Image Scaling

Wallpaper images are automatically scaled using the "cover" mode (similar to CSS `background-size: cover`):

- The image is scaled to cover the entire output
- Aspect ratio is maintained
- Excess portions are cropped if aspect ratios don't match
- The image is centered on the output

## Rotation Behavior

When rotation is enabled (`change_every_seconds > 0`):

- Each output has an independent rotation timer
- Images cycle in the order they appear in the configuration
- After reaching the last image, rotation loops back to the first
- Rotation continues automatically in the background

## Examples

### Single Static Wallpaper

```yaml
wallpapers:
  - name: "desktop-bg"
    wallpaper: "/usr/share/backgrounds/my-wallpaper.png"
    change_every_seconds: 0

monitor-groups:
  - name: "Default"
    monitors:
      - display: "DP-1"
        wallpaper: "desktop-bg"
```

### Rotating Slideshow

```yaml
wallpapers:
  - name: "slideshow"
    wallpaper:
      - "~/Pictures/slide1.jpg"
      - "~/Pictures/slide2.jpg"
      - "~/Pictures/slide3.jpg"
    change_every_seconds: 30  # Change every 30 seconds

monitor-groups:
  - name: "Presentation"
    monitors:
      - display: "HDMI-A-1"
        wallpaper: "slideshow"
```

### Folder-Based Wallpapers

```yaml
wallpapers:
  - name: "nature-collection"
    wallpaper: "~/Pictures/Nature/"  # All images in this folder
    change_every_seconds: 300  # Change every 5 minutes

monitor-groups:
  - name: "Home"
    monitors:
      - display: "eDP-1"
        wallpaper: "nature-collection"
```

### Per-Monitor Different Wallpapers

```yaml
wallpapers:
  - name: "laptop-bg"
    wallpaper: "~/Pictures/laptop.png"
    change_every_seconds: 0
    
  - name: "monitor-bg"
    wallpaper: "~/Pictures/monitor.png"
    change_every_seconds: 0

monitor-groups:
  - name: "Dual Setup"
    monitors:
      - display: "eDP-1"
        wallpaper: "laptop-bg"  # Laptop gets one wallpaper
        position: [0, 0]
      
      - display: "HDMI-A-1"
        wallpaper: "monitor-bg"  # External monitor gets another
        position: [1920, 0]
```

## Technical Details

{{< version-feature since="v0.1.0" >}}

### Implementation

- Images are loaded using **gdk-pixbuf** (supports PNG, JPEG, BMP, WebP)
- Rendering is done with **Cairo** for high-quality scaling
- Wallpapers are rendered to the **background layer** of the scene graph
- Each output manages its own wallpaper buffer and rotation timer

### Performance

- Images are scaled once when loaded
- Buffers are shared memory (SHM) based
- No performance impact on window rendering
- Rotation uses efficient timer-based updates

{{< /version-feature >}}

## Troubleshooting

### Wallpaper Not Showing

1. Check the log file for errors: `tail -f ~/.local/share/leviathan/leviathan.log`
2. Verify the image file exists and is readable
3. Ensure the wallpaper name matches between `wallpapers` and `monitor` configuration
4. Check that the image format is supported (PNG, JPEG, BMP, WebP)

### Rotation Not Working

1. Verify `change_every_seconds` is greater than 0
2. Ensure you have multiple wallpaper images configured
3. Check for errors in the log related to timer creation

### Image Looks Distorted

The wallpaper system uses "cover" scaling mode which maintains aspect ratio. If your image aspect ratio doesn't match your output:
- The image will be scaled to cover the entire output
- Excess portions will be cropped
- This is by design to avoid distortion

{{< hint warning >}}
**Note:** Very large images (> 4K) may take longer to load. Consider resizing images to your output resolution for faster loading.
{{< /hint >}}

## See Also

- [Configuration Reference]({{< relref "/docs/getting-started/configuration" >}})
- [Status Bar Configuration]({{< relref "/docs/features/status-bar" >}})
