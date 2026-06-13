# cboomer

Zoomer application for Linux - a fullscreen screenshot viewer with pan, zoom, flashlight effects, and live window tracking.

Originally written in [Nim](https://github.com/nim-lang/Nim), this is a C11 rewrite using X11 and OpenGL.

## Dependencies

### Debian / Ubuntu

```console
$ sudo apt install libgl1-mesa-dev libx11-dev libxext-dev libxrandr-dev
```

### Arch

```console
$ sudo pacman -S mesa libx11 libxext libxrandr
```

### Fedora

```console
$ sudo dnf install mesa-libGL-devel libX11-devel libXext-devel libXrandr-devel
```


## Quick Start

```console
$ make
$ ./cboomer --help
$ ./cboomer
```

This captures the entire screen and opens a fullscreen overlay. Drag to pan, scroll to zoom.

## Build Variants

| Command | Description |
|---|---|
| `make` | Release build. Shaders are embedded in the binary. |
| `make dev` | Developer build. Enables `Ctrl+R` shader hot-reload. |
| `make live` | Live image update - refreshes the screenshot every frame. |
| `make mitshm` | Live update using MIT-SHM extension (faster, use with `live`). |
| `make select` | Click on a window to track it instead of the whole screen. |

Multiple flags can be combined:

```console
$ make dev live mitshm select
```

## Controls

| Control | Description |
|---|---|
| Drag with left mouse button | Pan the image around |
| Scroll wheel | Zoom in / out |
| `=` / `-` | Zoom in / out |
| `0` | Reset position, scale, velocity, and mirror |
| `q` / `Esc` | Quit |
| `r` | Reload configuration file |
| `m` | Mirror the image horizontally |
| `f` | Toggle flashlight effect |
| `t` | Cycle shader: Normal, Invert, CRT, Grayscale, Edge |
| `Ctrl` + scroll / `+` / `-` | Adjust flashlight radius (when flashlight is on) |
| `Ctrl` + `r` | Reload shaders from disk (developer build only) |

## Command-Line Options

```
cboomer  -  fullscreen screenshot viewer

Usage:  cboomer [OPTIONS]

Options:
  -d, --delay <seconds>    delay execution by <seconds>
  -h, --help               show this help and exit
      --new-config [path]  generate a default config at [path]
  -c, --config <path>      use config at <path>
  -V, --version            show version and exit
  -w, --windowed           windowed mode instead of fullscreen
```

## Configuration

Config file is at `$HOME/.config/cboomer/config`. Generate a default one:

```console
$ cboomer --new-config
```

Format:

```
min_scale = 0.01
scroll_speed = 1.50
drag_friction = 6.00
scale_friction = 4.00
```

| Parameter | Default | Description |
|---|---|---|
| `min_scale` | `0.01` | Minimum zoom level |
| `scroll_speed` | `1.5` | Zoom speed per scroll tick |
| `drag_friction` | `6.0` | How quickly panning momentum decays |
| `scale_friction` | `4.0` | How quickly zoom momentum decays |

## How It Works

1. **Screenshot capture**: Uses `XGetImage` (or `XShmGetImage` with MIT-SHM) to grab the current screen contents into an `XImage`.
2. **OpenGL rendering**: The screenshot is uploaded as a `GL_TEXTURE_2D` and rendered onto a fullscreen quad. The vertex shader transforms coordinates based on camera position/scale, and the fragment shader applies effects. Shader modes (Normal, Invert, CRT, Grayscale, Edge) can be cycled with `t`. Each fragment shader preserves the flashlight and mirror features.
3. **Event loop**: Listens for X11 events (mouse, keyboard, close) to update camera state, flashlight, and mirror mode. Camera velocity decays over time for smooth inertia.
4. **Live mode** (`make live`): Refreshes the screenshot each frame, re-uploading the texture and updating the vertex buffer if the tracked window resizes.

## Shaders

All shaders live in `src/shaders/` as GLSL 1.30 source files. At build time the Makefile converts them into C string constants embedded in the binary via `build/shaders.h`, so no external shader files are needed at runtime.

### Vertex shader (`vert.glsl`)

Transforms vertex positions to implement camera pan and zoom. It converts screenshot coordinates into normalized device coordinates, applying `cameraPos`, `cameraScale`, and `windowSize` / `screenshotSize` ratio corrections to handle non-square aspect ratios.

### Fragment shaders

All fragment shaders share the same uniforms (`tex`, `cursorPos`, `windowSize`, `flShadow`, `flRadius`, `cameraScale`, `mirror`) and implement the flashlight and mirror features consistently:

* **Normal** (`frag.glsl`): Samples the texture directly. The flashlight darkens pixels outside a cursor-centered circle using `mix()` between the texel color and black, controlled by `flShadow` and `flRadius`. Mirror flips the x-axis of the texture coordinate.
* **Invert** (`frag_invert.glsl`): Same as Normal, but subtracts each color channel from 1.0 to produce a photographic negative.
* **CRT** (`frag_crt.glsl`): Adds a retro monitor look with three effects: chromatic aberration (shifts red and blue channels radially near edges), scanlines (a sine wave subtracted from the brightness at each row), and vignette (darkens corners using quadratic falloff from center).
* **Grayscale** (`frag_grayscale.glsl`): Converts to grayscale using luminance weights `dot(texel.rgb, vec3(0.299, 0.587, 0.114))`, then applies the flashlight on top.
* **Edge** (`frag_edge.glsl`): Runs a Sobel operator over the grayscale image using 9 texture lookups. It computes horizontal and vertical gradients (`gx`, `gy`) and outputs `sqrt(gx*gx + gy*gy)` as the edge intensity. Uses `textureSize()` to determine texel offsets instead of a uniform.

### Developer hot-reload

When built with `make dev`, pressing Ctrl+r at runtime re-reads all `.glsl` files from disk, recompiles both the vertex and all five fragment shaders, and re-links every program. This allows live iteration on shader code without restarting the viewer.

## Credits

- Original [Nim implementation](https://github.com/tsoding/boomer) by [Tsoding](https://twitch.tv/tsoding)
- X11/GLX reference: [Programming OpenGL in Linux](https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib)
- [Edge Detection Shaders](https://www.flyriver.com/g/edge-detection-shaders?auth=1781341835102)

## License

[MIT](https://opensource.org/license/MIT)
See the [LICENSE](LICENSE) file
