# ClockWidget - Time Display Plugin

A simple clock widget plugin for LeviathanDM status bar that displays the current time.

## Features

- Configurable time format (strftime)
- Adjustable update interval
- Customizable font size
- Thread-safe background updates
- Lightweight and efficient

## Building

### Quick Build

```bash
./build.sh
```

### Manual Build

```bash
mkdir build
cd build
cmake ..
make
```

## Installation

### User Installation (Recommended)

```bash
# After building
mkdir -p ~/.config/leviathan/plugins
cp build/clock-widget.so ~/.config/leviathan/plugins/
```

### System Installation

```bash
cd build
sudo make install  # Installs to /usr/local/lib/leviathan/plugins
```

## Configuration

Add to your `~/.config/leviathan/leviathan.yaml`:

```yaml
plugins:
  list:
    - name: ClockWidget
      config:
        format: "%H:%M:%S"           # Time format (see below)
        update_interval: "1"         # Update every 1 second
        font_size: "12"              # Font size in pixels
```

### Time Format

The `format` option uses strftime format strings. Common formats:

| Format | Example | Description |
|--------|---------|-------------|
| `%H:%M:%S` | `14:30:45` | 24-hour with seconds |
| `%H:%M` | `14:30` | 24-hour without seconds |
| `%I:%M %p` | `02:30 PM` | 12-hour with AM/PM |
| `%I:%M:%S %p` | `02:30:45 PM` | 12-hour with seconds and AM/PM |
| `%a %H:%M` | `Mon 14:30` | Day of week + time |
| `%b %d %H:%M` | `Jan 15 14:30` | Month, day, time |
| `%Y-%m-%d %H:%M` | `2025-01-15 14:30` | Full date and time |

See `man strftime` for all format options.

### Multiple Clocks

You can add multiple clock instances with different formats:

```yaml
plugins:
  list:
    # Local time
    - name: ClockWidget
      config:
        format: "%H:%M:%S"
        update_interval: "1"
        font_size: "12"
    
    # Date
    - name: ClockWidget
      config:
        format: "%a %b %d"
        update_interval: "60"
        font_size: "12"
```

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `format` | string | `%H:%M:%S` | Time display format (strftime) |
| `update_interval` | string | `1` | Seconds between updates |
| `font_size` | string | `12` | Font size in pixels |

## Development

### Requirements

- C++17 compiler
- CMake 3.15+
- Cairo development files
- LeviathanDM headers

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake libcairo2-dev

# Build
./build.sh
```

### Code Structure

```
clock-widget/
├── ClockWidget.cpp   - Plugin implementation
├── CMakeLists.txt    - Build configuration
├── build.sh          - Build script
└── README.md         - This file
```

## How It Works

1. **Background Thread**: Updates time every second (or configured interval)
2. **Thread-Safe**: Uses mutex to protect time string
3. **Dirty Flag**: Only re-renders when time changes
4. **Cairo Rendering**: Draws text on status bar

## License

Same as LeviathanDM

## Author

LeviathanDM Development Team

## See Also

- [Plugin Build Guide](../BUILD_GUIDE.md) - How to build plugins
- [Plugin Development Guide](../PLUGIN_DEV_GUIDE.md) - Creating custom plugins
- [Plugin Configuration](../../docs/PLUGIN_CONFIG.md) - Configuring plugins
