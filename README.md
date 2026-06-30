# cboomer [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/DavidBalishyan/cboomer)
>[!NOTE]
>There is an official mirror of this project on codeberg: <https://codeberg.org/DavidBalishyan/cboomer>

Zoomer application for Linux - a fullscreen screenshot viewer with pan, zoom, flashlight effects, and live window tracking.

Originally written in [Nim](https://github.com/nim-lang/Nim), this is a C11 rewrite using X11 and OpenGL.

[Watch the demo here](https://drive.google.com/file/d/1IT6zDoXzUNzzqA24JMwDpWog-PqxL2lA/view?usp=sharing)

## Dependencies

Install build dependencies (pick one):

**Option 1 - install-deps script**:

```console
$ ./scripts/install-deps.sh
```

**Option 2(recommended) - [envforge](https://github.com/DavidBalishyan/envforge)** (also manages env vars, build commands, and cleanup):

```console
$ envforge enter cboomer
```

[envforge](https://github.com/DavidBalishyan/envforge) reads the
`.envforge.yaml` in this repo: it installs the same packages, sets `CC` and
`CFLAGS`, runs `make`, and removes the packages when you exit the subshell.


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

### Experimental: Ninja build
>[!NOTE]
>See more at [scripts/experimental/README.md](scripts/experimental/README.md)

[Ninja](https://ninja-build.org/) build files can be generated for faster incremental builds and automatic header dependency tracking:

```console
# Perl (no extra dependencies)
$ perl scripts/experimental/generate.pl [--dev] [--live] [--mitshm] [--select]
$ ninja

# Python
$ python3 scripts/experimental/generate.py [--dev] [--live] [--mitshm] [--select]
$ ninja
```

Same feature flags as `make`. The primary build system is still Make; **this is experimental**.

## Scripts

| Script | Description |
|---|---|
| `scripts/install-deps.sh` | Install build dependencies (auto-detects distro) |
| `scripts/shader.sh` | Scaffold or remove a fragment shader (`help` for usage) |
| `scripts/lint.sh` | Run static analysis via cppcheck or clang-tidy |
| `scripts/release.sh` | Tag, push, and create a GitHub release from CHANGELOG |
| `scripts/gen_shaders.sh` | Generate `build/shaders.h` from `.glsl` files (called by Makefile) |
| `scripts/experimental/generate.pl` | Generate `build.ninja` (experimental, Perl) |
| `scripts/experimental/generate.py` | Generate `build.ninja` (experimental, Python) |

## Controls

| Control | Description |
|---|---|
| Drag with left mouse button | Pan the image around |
| Scroll wheel | Zoom in / out |
| `=` / `-` | Zoom in / out |
| `0` | Reset position, scale, velocity, and mirror (smoothly if `smooth_reset` is enabled) |
| `1` `2` `3` `4` `5` | Zoom presets: 100%, 200%, 400%, 800%, 1600% (smooth when `smooth_reset` is enabled) |
| `←` `→` `↑` `↓` / `h` `j` `k` `l` | Pan image |
| `Ctrl`+`f`/`b`/`n`/`p` | Pan image (small step, Emacs-style) |
| `i` | Toggle texture filtering: Nearest (pixel-perfect) <-> Linear (smooth) |
| `o` | Toggle on-screen display (shader, zoom, FPS, color under cursor) |
| `q` / `Esc` | Quit |
| `r` | Reload configuration file |
| `m` | Mirror the image horizontally |
| `f` | Toggle flashlight effect |
| `t` | Cycle shader: Normal, Invert, CRT, Grayscale, Edge, VHS Glitch, Distortion, Zoom Blur, Posterize, Pixelate, Sepia, Emboss |
| `s` | Save current view (with shaders applied) to `cboomer_<timestamp>.ppm` or `.png` (see `screenshot_format`) |
| `Ctrl` + scroll / `+` / `-` | Adjust flashlight radius (when flashlight is on) |
| `Ctrl` + `[` / `]` | Rotate image 90 degrees counter-clockwise / clockwise |
| `Ctrl` + `r` | Reload shaders from disk (developer build only) |

## Command-Line Options
After installing, run: 
```console
$ cboomer --help
```

## Configuration

Config file is at `$HOME/.config/cboomer/config`. Generate a default one:

```console
$ cboomer --new-config
```

Format:

```
key = "value" # or values can have different types
```
Values are typed by their literal form:

| Example | Type |
|---|---|
| `"hello"` | string |
| `'x'` | char |
| `42` | int |
| `3.14` | double |
| `true` / `false` | bool |

Lines starting with `#` and inline comments after `#` are ignored. Boolean values accept `true`/`false`, `1`/`0`, `yes`/`no`, and `on`/`off`. Path values accept `$HOME/...` and `~/...` - both expand to the user's home directory.

| Parameter | Default | Description |
|---|---|---|
| `min_scale` | `0.01` | Minimum zoom level |
| `scroll_speed` | `1.5` | Zoom speed per scroll tick |
| `drag_friction` | `6.0` | How quickly panning momentum decays |
| `scale_friction` | `4.0` | How quickly zoom momentum decays |
| `ppm_save_path` | `$HOME/.config/cboomer/screenshot.ppm` | Path to save a PPM screenshot on startup |
| `ppm_save` | `false` | When `true`, saves a PPM screenshot to `ppm_save_path` on startup |
| `default_shader` | `"Normal"` | Default shader on startup (one of the shader names listed below) |
| `mirror` | `false` | Start with mirror mode enabled |
| `flashlight_radius` | `200.0` | Initial flashlight radius in pixels |
| `scroll_invert` | `false` | Invert scroll-to-zoom direction |
| `osd` | `false` | Start with on-screen display visible (shader, zoom, FPS); toggle anytime with `o` |
| `smooth_reset` | `true` | Smooth animation when resetting with `0` |
| `font` | *(auto-detect)* | Path to a .ttf/.otf font for the OSD (full path or font name to search system); empty means auto-detect |
| `screenshot_dir` | `~/Pictures/Screenshots` | Directory for rendered view saves (press `s`); supports `~/` expansion |
| `screenshot_format` | `"ppm"` | Image format for `s` key saves: `"ppm"` or `"png"` |

## How It Works

1. **Screenshot capture**: Uses `XGetImage` (or `XShmGetImage` with MIT-SHM) to grab the current screen contents into an `XImage`.
2. **OpenGL rendering**: The screenshot is uploaded as a `GL_TEXTURE_2D` and rendered onto a fullscreen quad. The vertex shader transforms coordinates based on camera position/scale, and the fragment shader applies effects. 12 shader modes can be cycled with `t`. Each fragment shader preserves the flashlight and mirror features.
3. **Event loop**: Listens for X11 events (mouse, keyboard, close) to update camera state, flashlight, and mirror mode. Camera velocity decays over time for smooth inertia.
4. **Live mode** (`make live`): Refreshes the screenshot each frame, re-uploading the texture and updating the vertex buffer if the tracked window resizes.

## Shaders

All shaders live in `src/shaders/` as GLSL source files. At build time the Makefile converts them into C string constants embedded in the binary via `build/shaders.h`, so no external shader files are needed at runtime. This includes the OSD shaders (`osd_vert.glsl`, `osd_frag.glsl`) alongside the main rendering pipeline.
Shaders that use animation receive a `time` uniform (elapsed seconds since launch). Non-animated shaders silently ignore it (OpenGL ignores `glUniform` for non-existent locations).

### Vertex shader (`vert.glsl`)

Transforms vertex positions to implement camera pan and zoom. It converts screenshot coordinates into normalized device coordinates, applying `cameraPos`, `cameraScale`, and `windowSize` / `screenshotSize` ratio corrections to handle non-square aspect ratios.

### Fragment shaders

All fragment shaders share the same uniforms (`tex`, `cursorPos`, `windowSize`, `flShadow`, `flRadius`, `cameraScale`, `mirror`) and implement the flashlight and mirror features consistently:

* **Normal** (`frag.glsl`): Samples the texture directly. The flashlight darkens pixels outside a cursor-centered circle using `mix()` between the texel color and black, controlled by `flShadow` and `flRadius`. Mirror flips the x-axis of the texture coordinate.
* **Invert** (`frag_invert.glsl`): Same as Normal, but subtracts each color channel from 1.0 to produce a photographic negative.
* **CRT** (`frag_crt.glsl`): Adds a retro monitor look with three effects: chromatic aberration (shifts red and blue channels radially near edges), scanlines (a sine wave subtracted from the brightness at each row), and vignette (darkens corners using quadratic falloff from center).
* **Grayscale** (`frag_grayscale.glsl`): Converts to grayscale using luminance weights `dot(texel.rgb, vec3(0.299, 0.587, 0.114))`, then applies the flashlight on top.
* **Edge** (`frag_edge.glsl`): Runs a Sobel operator over the grayscale image using 9 texture lookups. It computes horizontal and vertical gradients (`gx`, `gy`) and outputs `sqrt(gx*gx + gy*gy)` as the edge intensity. Uses `textureSize()` to determine texel offsets instead of a uniform.
* **VHS Glitch** (`frag_vhsglitch.glsl`): Animated retro VCR corruption. Random horizontal block shifts (using a screen-space hash function) displace entire row sections. Red and blue channels are offset independently for chromatic separation. Occasional noise bars and white dropout lines flicker in and out. All timing driven by the `time` uniform.
* **Distortion** (`frag_distortion.glsl`): Combines an animated sine-wave ripple (60 waves radiating from center, oscillating at 4 rad/s) with a subtle barrel distortion (quadratic warp toward edges). The two effects are blended at 30/70 ratio.
* **Zoom Blur** (`frag_zoomblur.glsl`): Radial motion blur originating from the cursor position. For each fragment it takes 24 samples along the direction from the cursor to the pixel, spaced with quadratic falloff, and averages them. Blur intensity increases with distance from the cursor.
* **Posterize** (`frag_posterize.glsl`): Reduces each color channel to 4 discrete levels (`floor(texel * 4) / 4`), creating a flat graphic-art/comic-book look.
* **Pixelate** (`frag_pixelate.glsl`): Divides the texture into a coarse grid (cell size 0.02 in UV space, ~50 cells across) and samples once at the center of each cell. Simple `floor` + offset on UV coordinates.
* **Sepia** (`frag_sepia.glsl`): Applies the classic darkroom sepia tone using weighted dot products: `R = dot(rgb, (0.393, 0.769, 0.189))`, `G = dot(rgb, (0.349, 0.686, 0.168))`, `B = dot(rgb, (0.272, 0.534, 0.131))`.
* **Emboss** (`frag_emboss.glsl`): A simplified emboss kernel comparing the grayscale luminance of the top-left and bottom-right corners of a 3x3 neighborhood. The difference `grayBR - grayTL + 0.5` produces a 3D relief effect with a neutral gray background.

### Developer hot-reload

When built with `make dev`, pressing Ctrl+r at runtime re-reads all `.glsl` files from disk, recompiles both the vertex and all fragment shaders, and re-links every program. This allows live iteration on shader code without restarting the viewer.

## Editor Support

See [`editors/README.md`](editors/README.md) for install instructions.

Syntax highlighting for the config file is available for **Vim** (`editors/vim/cboomer.vim`) and **Emacs** (`editors/emacs/cboomer-mode.el`). Both highlight keys, strings, numbers, booleans, and comments.

## Referances

- Original [Nim implementation](https://github.com/tsoding/boomer) by [Tsoding](https://twitch.tv/tsoding), [Livestream(s)](https://www.twitch.tv/collections/HlRy-q69uBXmpQ)
- X11/GLX reference: [Programming OpenGL in Linux](https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib)
- [stb single-file public domain libraries](https://github.com/nothings/stb)
- [Edge Detection Shaders](https://www.flyriver.com/g/edge-detection-shaders?auth=1781341835102)
- [OpenGL documentation](https://docs.gl)
- [Elisp documentation](https://emacsdocs.org/docs/elisp/Emacs-Lisp)
- [Vimscript page](https://vimhelp.org/usr_41.txt.html)
- <https://perldoc.perl.org>
- <https://www.python.org>
- <https://ninja-build.org/manual.html>
- <https://netpbm.sourceforge.net/doc/ppm.html>
- <https://www.x.org/releases/X11R7.7/doc/xextproto/shm.html>
- <https://pcpartsgeek.com/bilinear-filtering>
- <https://www.geeksforgeeks.org/data-science/manhattan-distance>
- <https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexParameter.xhtml>

## License

[GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.html)
See the [LICENSE](LICENSE) file
