# Battery Widget

A comprehensive battery monitoring widget for LeviathanDM that uses UPower via DBus to monitor:
- Main laptop/device battery
- Bluetooth peripherals (headsets, mice, keyboards, controllers)
- UPS devices
- Any other battery-powered devices connected to your system

## Features

- **Real-time monitoring**: Automatically updates battery status using UPower's DBus interface
- **Popover interface**: Click the widget to see all battery-powered devices
- **Color-coded warnings**: Visual indicators for low and critical battery levels
- **Device icons**: Nerd Font icons for different device types
- **Time remaining**: Optional display of charging/discharging time
- **Bluetooth support**: Monitors battery levels of connected Bluetooth devices

## Dependencies

- UPower (`org.freedesktop.UPower` on system DBus)
- GLib/GIO for DBus communication
- Cairo for rendering
- Nerd Fonts for icons

## Configuration

Add to your `leviathanrc` in the status bar `widgets` section:

```toml
[[status_bars.widgets]]
type = "BatteryWidget"
section = "right"

# Optional configuration
[status_bars.widgets.config]
show_percentage = true                # Show percentage text (default: true)
show_time = false                     # Show time remaining (default: false)
low_battery_threshold = 20            # Low battery warning level (default: 20)
critical_battery_threshold = 10       # Critical battery level (default: 10)
update_interval = 30                  # Update interval in seconds (default: 30)
font_size = 14                        # Font size for text (default: 12)
text_color = "#ECEFF4"               # Text color (default: white)
```

## Usage

### Basic Display
The widget shows your main battery status with an icon and optional percentage:
- 󰁹 90%+ (Full)
- 󰂂 80%+ 
- 󰁼 20%+
- 󰁺 <10% (Critical - shown in red)
- 󰂄 Charging (shown with charging icon)

### Popover
Click the widget to open a popover showing:
- Main battery with current percentage
- All connected battery-powered devices
- Device type icons (mouse, keyboard, headset, etc.)
- Battery percentage for each device

### Color Coding
- **White**: Normal battery level (>20%)
- **Orange**: Low battery (<20%)
- **Red**: Critical battery (<10%)

## Device Types Supported

The widget recognizes and shows appropriate icons for:

| Device Type | Icon | Description |
|------------|------|-------------|
| Battery | 󰁹 | Main laptop battery |
| Mouse | 󰍽 | Wireless mouse |
| Keyboard | 󰌌 | Wireless keyboard |
| Headset/Headphones | 󰋋 | Bluetooth audio devices |
| Gaming Controller | 󰖺 | Game controllers |
| Phone | 󰄜 | Connected phones |
| And more... | | See UPower device types |

## DBus Integration

This widget uses the `DBusHelper` base class to communicate with UPower:

```
Bus: org.freedesktop.UPower
Object: /org/freedesktop/UPower
Interface: org.freedesktop.UPower

Devices: /org/freedesktop/UPower/devices/*
Interface: org.freedesktop.UPower.Device
```

### Monitored Signals
- `DeviceAdded`: When a new battery device is connected
- `DeviceRemoved`: When a battery device is disconnected
- Property changes on devices (percentage, state, etc.)

## Building

The plugin is built automatically with the main project. To build standalone:

```bash
cd plugins/battery-widget
mkdir build && cd build
cmake ..
make
```

The plugin will be installed to `/usr/local/lib/leviathan/plugins/libbattery-widget.so`

## Troubleshooting

### Widget shows "--%" or doesn't update
- Check if UPower is running: `systemctl status upower`
- Verify DBus connection: `dbus-send --system --dest=org.freedesktop.UPower --print-reply /org/freedesktop/UPower org.freedesktop.UPower.EnumerateDevices`

### No Bluetooth devices shown
- Ensure Bluetooth devices are paired and connected
- Check if UPower detects them: `upower -d`
- Some devices may not report battery status

### Permissions
The widget uses the system DBus bus which requires UPower to be running. No special permissions are needed as UPower is accessible to all users.

## Example Popover Layout

```
┌─────────────────────────────┐
│ 󰁹 Main Battery         87% │
├─────────────────────────────┤
│ 󰋋 Sony WH-1000XM4      65% │
│ 󰍽 Logitech MX Master   42% │
│ 󰌌 Keychron K2          89% │
└─────────────────────────────┘
```

## Extending

The `DBusHelper` base class can be used to create widgets for other DBus services:

- NetworkManager for WiFi status
- BlueZ for Bluetooth management
- SystemD for system stats
- Notifications (org.freedesktop.Notifications)
- MediaPlayer2 for media controls

See `/include/ui/DBusHelper.hpp` for the full API.

## License

Part of LeviathanDM - see main project LICENSE
