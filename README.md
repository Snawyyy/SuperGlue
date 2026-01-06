# SuperGlue
A lightweight, high-performance window decoration plugin for Hyprland. SuperGlue acts as a rendering backend for external scripts, allowing you to draw transient icons, status indicators, and dynamic primitives (like tether lines) directly onto windows.

## Purpose
SuperGlue separates the **visuals** from the **logic**. It has no internal concept of audio changes or mouse inputs. instead, it listens for rendering commands from external tools via a file-based IPC. This keeps your Hyprland compositor lightweight while enabling rich visual feedback for your scripts.

### Capabilities
- **Texture Rendering**: Efficiently loads, caches, and renders PNG assets (including pixel art) from `~/.icons/`.
- **Overlay Management**: Supports transient overlays (like volume or mute status) with built-in fade-out animations.
- **Dynamic Primitives**: Renders vector graphics, such as the dynamic "tether" line used for autoscroll indicators.
- **IPC Interface**: A low-latency file watcher that accepts commands from any language (Shell, Python, Rust, etc.).

## Usage
Control SuperGlue by writing commands to `/tmp/superglue_cmd`.

### Supported Commands

#### Volume & Status
Draws a transient icon that fades out after a short duration.
```bash
# Render a volume-up icon on a specific window with opacity 0.8
echo "vol-up <window_address> 80" > /tmp/superglue_cmd

# Render a mute icon (toggles visibility)
echo "mute-toggle <window_address>" > /tmp/superglue_cmd
```

#### Scroll Anchors
Draws a persistent anchor point and a dynamic dotted line connecting it to the mouse cursor.
```bash
# Start an anchor at specific screen coordinates
echo "scroll-start <window_address> 1920 1080" > /tmp/superglue_cmd

# Remove the anchor
echo "scroll-stop <window_address>" > /tmp/superglue_cmd
```

## Installation

### Prerequisites
- Hyprland headers (usually provided by `hyprland-devel` or by checking out the Hyprland repo)
- CMake
- Ninja (optional, for faster builds)

### Building
```bash
mkdir build && cd build
cmake ..
make
```

### Loading
Add the plugin to your Hyprland configuration:
```bash
plugin = /absolute/path/to/superglue.so
```

## Architecture
SuperGlue attaches a `Superglue` decoration object to every window managed by the compositor. This object hooks into the render loop to draw overlays on top of the window content but below the compositor's strict overlay layer (like lockscreens), ensuring it feels integrated into the desktop environment.
