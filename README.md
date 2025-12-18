# LeviathanDM - Wayland Compositor

A lightweight, customizable tiling Wayland compositor built with wlroots and C++.

## Features

- **Multiple Tiling Layouts**:
  - Master-Stack (similar to dwm/xmonad)
  - Monocle (fullscreen stacking)
  - Grid layout
  
- **9 Workspaces**: Quick workspace switching and window management

- **Customizable**: Colors, gaps, borders, keybindings

- **Wayland Native**: Modern display protocol with wlroots

- **Vim-like Keybindings**: Familiar navigation (hjkl)

- **Layer-Shell Protocol**: Support for panels, notifications, and overlays (wlr-layer-shell-v1)

- **Built-in Help**: Press `Super + F1` for an overlay showing all keybindings

## Building

### Dependencies

LeviathanDM uses **vendored dependencies** (bundled with the source) for core components:
- **wlroots 0.19.2** - Compositor library
- **libdisplay-info 0.2.0** - Monitor EDID parsing

You only need to install build tools and base libraries:

**Required:**
- CMake 3.15+
- C++17 compiler (g++ or clang++)
- meson and ninja (for building libdisplay-info)
- wayland
- wayland-protocols  
- xkbcommon
- pixman
- GTK4 (for help window)
- gtk-layer-shell (for help window overlay)

On Ubuntu/Debian:
```bash
sudo apt install build-essential cmake meson ninja-build \
  wayland-protocols libwayland-dev libxkbcommon-dev libpixman-1-dev \
  libgtk-4-dev libgtk-layer-shell-dev
```

On Arch Linux:
```bash
sudo pacman -S base-devel cmake meson ninja \
  wayland wayland-protocols xkbcommon pixman \
  gtk4 gtk-layer-shell
```

On Fedora:
```bash
sudo dnf install gcc-c++ cmake meson ninja-build \
  wayland-devel wayland-protocols-devel \
  libxkbcommon-devel pixman-devel \
  gtk4-devel gtk-layer-shell-devel
```

### Compile

The build script automatically downloads and builds vendored dependencies:

```bash
./build.sh
```

This will:
1. Download wlroots 0.19.2 and libdisplay-info 0.2.0 (first time only)
2. Build vendored dependencies
3. Build LeviathanDM
4. Create `build/leviathan` binary

**Manual build:**
```bash
./setup-deps.sh      # Download dependencies
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Install

```bash
sudo make install
```

## Running

### From TTY

The compositor runs directly on your hardware (no X11 needed):

```bash
# Switch to a TTY (Ctrl+Alt+F2)
# Login and run:
./build/leviathan
```

### From Existing Session

You can test in a nested Wayland session using:

```bash
# Install cage (nested Wayland compositor)
sudo apt install cage  # Ubuntu/Debian
sudo pacman -S cage    # Arch

# Run nested
cage ./build/leviathan
```

Or use weston:
```bash
weston --backend=wayland-backend.so &
WAYLAND_DISPLAY=wayland-1 ./build/leviathan
```

### As Display Manager Entry

Create `/usr/share/wayland-sessions/leviathan.desktop`:

```ini
[Desktop Entry]
Name=LeviathanDM
Comment=Tiling Wayland Compositor
Exec=leviathan
Type=Application
```

## Default Keybindings

All keybindings use `Super` (Windows key) as the modifier.

### Window Management
- `Super + Return` - Launch terminal (alacritty)
- `Super + Shift + c` - Close focused window
- `Super + Shift + q` - Quit compositor

### Navigation
- `Super + j` - Focus next window
- `Super + k` - Focus previous window
- `Super + Shift + j` - Swap with next window
- `Super + Shift + k` - Swap with previous window

### Layout Control
- `Super + h` - Decrease master area width
- `Super + l` - Increase master area width
- `Super + i` - Increase master window count
- `Super + d` - Decrease master window count

### Layout Switching
- `Super + t` - Master-stack layout
- `Super + m` - Monocle layout
- `Super + g` - Grid layout

### Workspaces
- `Super + [1-9]` - Switch to workspace 1-9
- `Super + Shift + [1-9]` - Move window to workspace 1-9

### Applications
- `Super + p` - Application launcher (rofi)
- `Super + F1` - Show keybinding help window

## Help Window

LeviathanDM includes a built-in help window that displays all keybindings. Press `Super + F1` to show it.

The help window:
- Uses the layer-shell protocol to appear as an overlay
- Groups keybindings by category for easy reference
- Can be dismissed with `Esc` or `F1`
- Features a dark theme matching the compositor aesthetic

The help tool is built with GTK4 and gtk-layer-shell, and is located at `build/tools/help-window/leviathan-help`.

## Configuration

Create `~/.config/leviathan/leviathanrc`:

```
# Border and gaps
border_width 2
gap_size 10

# Colors (hex format)
border_focused #5e81ac
border_unfocused #3b4252

# Workspaces
workspace_count 9

# Focus behavior
focus_follows_mouse true
```

## Architecture

- **Server**: Core compositor, wlroots backend, renderer, and scene graph
- **Output**: Monitor/display management
- **View**: XDG toplevel window management
- **Input**: Keyboard and pointer input handling
- **TilingLayout**: Layout algorithms (master-stack, grid, monocle)
- **Config**: Configuration file parsing
- **KeyBindings**: Keyboard shortcut management
- **Types**: Common data structures (View, Workspace, Output, etc.)

## Why Wayland?

Wayland is the modern display protocol that provides:
- Better security (app isolation)
- No screen tearing
- Better multi-monitor support
- Lower latency
- Hardware acceleration

Using **wlroots** gives us:
- Stable, battle-tested compositor foundation
- Used by sway, river, hyprland, and many others
- Handles rendering, input, and protocols
- Lets us focus on window management logic

## Customization

The compositor is designed to be easily hackable. Key files:

- `src/KeyBindings.cpp` - Modify keybindings
- `src/TilingLayout.cpp` - Add new layouts
- `src/Config.cpp` - Add configuration options
- `include/Types.hpp` - Adjust data structures
- `src/Server.cpp` - Core compositor logic

## Troubleshooting

**Compositor won't start**: Make sure you're running from a TTY or nested Wayland session, not X11.

**Black screen**: Check wlroots version (need 0.17+). Run with `WLR_RENDERER=vulkan` or `WLR_RENDERER=gles2`.

**No input**: Ensure your user is in the `input` and `video` groups:
```bash
sudo usermod -a -G input,video $USER
```

**Missing libraries**: Install wlroots development packages for your distro.

## License

MIT License - Feel free to modify and distribute!

## Contributing

Built out of frustration with existing window managers/compositors. Feel free to fork and customize!
