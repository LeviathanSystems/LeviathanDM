# System Monitor Widget

A comprehensive system monitoring widget that displays CPU, memory, swap, and disk usage information using native LeviathanDM UI components (Labels and HBox containers).

## Features

- **CPU Usage**: Real-time CPU utilization percentage
- **Memory Usage**: Physical RAM usage with percentage
- **Swap Usage**: Virtual memory usage (only shown when in use)
- **Disk Usage**: Per-mount disk space usage
- **Configurable**: Choose which metrics to display via configuration
- **Auto-updating**: Uses `PeriodicWidget` pattern for efficient background updates
- **Native UI**: Uses Label and HBox widgets for proper rendering

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Installation

```bash
# From the build directory:
make install

# Or manually:
cp build/system-monitor-widget.so ~/.config/leviathan/plugins/
```

## Configuration

Configure the widget in your `leviathan.yaml`:

### Show/Hide Metrics

```yaml
status_bar:
  widgets:
    - plugin: "system-monitor-widget.so"
      show_cpu: true        # Default: true
      show_memory: true     # Default: true
      show_swap: true       # Default: true
      show_disks: false     # Default: false
      disk_mounts: "/,/home"  # Comma-separated list
```

### Font and Colors

```yaml
status_bar:
  widgets:
    - plugin: "system-monitor-widget.so"
      update_interval: 2    # Update every 2 seconds (default: 1)
      font_size: 12         # Font size
      font_family: "monospace"
      text_color: "#FFFFFF" # White text
```

## Usage Examples

### CPU and Memory Only (Default)

```yaml
status_bar:
  widgets:
    - plugin: "system-monitor-widget.so"
```

Output: `CPU:15.3% | MEM:8.2G/16.0G(51%)`

### All Metrics

```bash
export LEVIATHAN_SYSMON_SHOW_CPU=1
export LEVIATHAN_SYSMON_SHOW_MEMORY=1
export LEVIATHAN_SYSMON_SHOW_SWAP=1
export LEVIATHAN_SYSMON_SHOW_DISKS=1
export LEVIATHAN_SYSMON_DISK_MOUNTS="/,/home"
```

Output: `CPU:15.3% | MEM:8.2G/16.0G(51%) | SWAP:512M(10%) | root:45G/100G(45%) | home:230G/500G(46%)`

### CPU Only

```bash
export LEVIATHAN_SYSMON_SHOW_CPU=1
export LEVIATHAN_SYSMON_SHOW_MEMORY=0
export LEVIATHAN_SYSMON_SHOW_DISKS=0
```

Output: `CPU:15.3%`

### Disk Space Only

```bash
export LEVIATHAN_SYSMON_SHOW_CPU=0
export LEVIATHAN_SYSMON_SHOW_MEMORY=0
export LEVIATHAN_SYSMON_SHOW_DISKS=1
export LEVIATHAN_SYSMON_DISK_MOUNTS="/,/home,/mnt/backup"
```

Output: `root:45G/100G(45%) | home:230G/500G(46%) | backup:800G/2000G(40%)`

## Output Format

### CPU
- Format: `CPU:XX.X%`
- Updates every 2 seconds
- Calculated from `/proc/stat`

### Memory
- Format: `MEM:X.XG/X.XG(XX%)`
- Shows: Used/Total(Percentage)
- Auto-scales: KB, MB, GB
- Calculated from `/proc/meminfo`

### Swap
- Format: `SWAP:X.XG(XX%)`
- Only shown when swap is actively in use
- Auto-scales: KB, MB, GB

### Disk
- Format: `mount:XXG/XXG(XX%)`
- Shows: Used/Total(Percentage)
- Mount names simplified (`/` → root, `/home` → home)
- Monitors multiple mounts simultaneously

## Implementation Details

- **Extends**: `PeriodicWidget` base class
- **Update Interval**: 2000ms (2 seconds)
- **Thread-Safe**: Background updates don't block compositor
- **Efficient**: Only reads `/proc` files when needed
- **CPU Calculation**: Delta between readings for accurate percentage
- **Memory Format**: Human-readable with appropriate units

## Dependencies

- Standard C++17
- Linux `/proc` filesystem
- POSIX `statvfs` for disk info
- PeriodicWidget base class

## Troubleshooting

### Widget shows "SYS:--"
- Check that the plugin is loaded: `ls ~/.config/leviathan/plugins/`
- Verify permissions: plugin file should be executable

### CPU always shows 0%
- Ensure `/proc/stat` is readable
- Widget needs at least 2 seconds between readings for delta calculation

### Disk not showing
- Set `LEVIATHAN_SYSMON_SHOW_DISKS=1`
- Verify mount points exist: `mount | grep -E "^/dev"`
- Check mount path spelling in `LEVIATHAN_SYSMON_DISK_MOUNTS`

### Swap not appearing
- Swap only shows when in use (swapUsed > 0)
- Check if swap is enabled: `swapon --show`
- Verify `/proc/meminfo` contains SwapTotal and SwapFree

## See Also

- [Battery Widget](../battery-widget/) - Similar periodic update pattern
- [Clock Widget](../clock-widget/) - Another example of PeriodicWidget
- [Plugin Development Guide](../PLUGIN_DEV_GUIDE.md)
