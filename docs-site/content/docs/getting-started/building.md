---
title: "Building from Source"
weight: 1
---

# Building from Source

## Dependencies

LeviathanDM uses **vendored dependencies** (bundled with source) for core components:
- **wlroots 0.19.2** - Compositor library
- **libdisplay-info 0.2.0** - Monitor EDID parsing

You only need to install build tools and base libraries.

### Ubuntu/Debian

```bash
sudo apt install build-essential cmake meson ninja-build \
  wayland-protocols libwayland-dev libxkbcommon-dev libpixman-1-dev \
  libgtk-4-dev libgtk-layer-shell-dev libglib2.0-dev libgio2.0-cil-dev \
  libcairo2-dev
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake meson ninja \
  wayland wayland-protocols xkbcommon pixman \
  gtk4 gtk-layer-shell glib2 cairo
```

### Fedora

```bash
sudo dnf install gcc-c++ cmake meson ninja-build \
  wayland-devel wayland-protocols-devel \
  libxkbcommon-devel pixman-devel \
  gtk4-devel gtk-layer-shell-devel \
  glib2-devel cairo-devel
```

## Build Steps

### Quick Build

The build script automatically handles everything:

```bash
./build.sh
```

This will:
1. Download wlroots 0.19.2 and libdisplay-info 0.2.0 (first time only)
2. Build vendored dependencies
3. Build LeviathanDM
4. Create `build/leviathan` binary

### Manual Build

For more control:

```bash
# Download dependencies
./setup-deps.sh

# Configure
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)
```

## Installation

To install system-wide:

```bash
cd build
sudo make install
```

This installs:
- `/usr/local/bin/leviathan` - Main binary
- `/usr/local/lib/leviathan/` - Plugin directory
- `/usr/local/share/leviathan/` - Resources

## Running

### From TTY

The compositor runs directly on hardware:

```bash
# Switch to TTY (Ctrl+Alt+F2)
# Login, then:
./build/leviathan
```

### Nested Session (for testing)

Run inside an existing Wayland session:

```bash
cage ./build/leviathan
```

Or X11:

```bash
WAYLAND_DISPLAY=wayland-1 ./build/leviathan
```

## Troubleshooting

### Permission Denied

If you get permission errors when running from TTY:

```bash
# Add yourself to input and video groups
sudo usermod -a -G input,video $USER
# Log out and back in
```

### Build Errors

If vendored dependencies fail to build:

```bash
# Clean and rebuild
rm -rf build
./build.sh
```

### Missing Dependencies

Check that all dependencies are installed:

```bash
pkg-config --modversion wayland-server
pkg-config --modversion xkbcommon
pkg-config --modversion pixman-1
```

## Next Steps

- [Configuration]({{< relref "/docs/getting-started/configuration" >}})
- [Keybindings]({{< relref "/docs/getting-started/keybindings" >}})
