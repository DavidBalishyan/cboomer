# [Semantic versioning](https://semver.org/)
Given a version number MAJOR.MINOR.PATCH, increment the:

- MAJOR version when you make incompatible API changes
- MINOR version when you add functionality in a backward compatible manner
- PATCH version when you make backward compatible bug fixes
Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format.
# v1.0.0

Zoomer application for Linux with *pan, zoom, flashlight, mirror, and live window tracking*. **C11** rewrite using **X11** and **OpenGL**.

- Screenshot capture via **XGetImage** / **XShmGetImage**
- OpenGL rendering with smooth pan/zoom inertia
- Flashlight mode *(f)* with adjustable radius
- Mirror mode *(m)*
- Config file at *$HOME/.config/cboomer/config*
- Build variants: *make dev, make live, make mitshm, make select*
- CLI flags: *--delay, --help, --new-config, --config, --version, --windowed*

Commit: [822a079](https://github.com/DavidBalishyan/cboomer/commit/822a0795fc8e2a80612b631c9d2543442f431c37)

# v1.1.0

## What's new
- **Shader effects!** Press *t* to cycle through 5 modes: Normal, Invert (photographic negative), CRT (chromatic aberration + scanlines + vignette), Grayscale, and Edge (Sobel operator).
- **Shaders moved to src/shaders** - all *.glsl* files live in their own directory, embedded as C strings at build time. 
- **README overhaul** - added demo video link, full shader documentation explaining every effect, and improved build variant table.
- **Build the binary to CWD** - *cboomer* now lands in the project root instead of *build/*.

Commit: [084b84f](https://github.com/DavidBalishyan/cboomer/commit/084b84fc45f76d57353ad1b7cdf954b035bdc18a)

# v1.2.0

## What's new
- **7 new shaders!** 12 total now. Added VHS Glitch (animated block shift + channel offset + noise), Distortion (ripple + barrel warp), Zoom Blur (radial blur from cursor), Posterize (4-level color quantization), Pixelate (low-res grid), Sepia (vintage photo tone), and Emboss (3D relief).
- **Animated shaders** - new `time` uniform passed to all fragment shaders enables real-time animation (VHS glitch, Distortion).
- **Shader cycling** wraps through all 12 modes with *t* key.

commit: [2deaacc](https://github.com/DavidBalishyan/cboomer/commit/2deaaccf86d6a9f12dfeff340247bc1fe28bf424)
# v1.2.1
>[!NOTE]
>This is a PATCH(bug fix version)
## Fixes
- **Fixed crash on shader hot-reload** (Ctrl+R in `DEVELOPER` mode) - `free()` called on embedded string constants in `src/main.c:103`.
- **Fixed corrupted rendering in LIVE builds** - vertex array had 5 floats per vertex but stride was set to 4 floats, causing GPU to read misaligned data (`src/main.c:722`).
- **Fixed heap buffer overflow in `reload_shader`** - unhandled `ftell` failure returned `-1`, which when cast to `size_t` produced `SIZE_MAX`, leading to a near-zero `malloc` followed by a massive `fread` (`src/main.c:107`).
- **Fixed MIT-SHM fallback on alloc failure** - `XShmCreateImage`, `shmget`, `shmat`, and `XShmAttach` were all unchecked; any failure caused a segfault or use of invalid shared memory (`src/screenshot.c:16-35`).
- **Fixed `XVisualInfo` leak** - `glXChooseVisual` result (`vi`) was never freed with `XFree` (`src/main.c:455`).
- **Fixed division by zero from config** - `min_scale` was accepted as 0 or negative, leading to `vec2_div_f` dividing by zero in navigation (`src/config.c:45`, `src/navigation.c:12-14`).
- **Fixed dangling pointer on screenshot refresh failure** - `XGetSubImage` partial corruption + `XGetImage` failure left `screenshot->image` in an undefined state (`src/screenshot.c:75-98`).
- **Fixed config parser abort** - unknown config keys called `exit(1)` instead of issuing a warning (`src/config.c:53`).
- **Fixed sleep not retried on signal** - `nanosleep` returned early on `EINTR` without retrying (`src/main.c:400`).
- **Fixed unchecked `XGetWindowAttributes`** - root window and event loop window attribute fetches could silently return garbage (`src/main.c:464,584`).
- **Fixed unchecked `XGrabPointer`/`XGrabKeyboard`** - grab failures in `SELECT` mode were ignored, allowing the event loop to hang (`src/main.c:221-227`).
- **Fixed uninitialized shader source on partial `fread`** - `reload_shader` didn't check how many bytes were actually read (`src/main.c:111`).
- **Fixed memory leak on exit in DEVELOPER mode** - reloaded shader heap buffers were never freed (`src/main.c:743`).
- **Fixed undetected `snprintf` truncation** - config path construction silently used truncated paths (`src/main.c:308,311,350,388`).

commit: [2deaacc](https://github.com/DavidBalishyan/cboomer/commit/2deaaccf86d6a9f12dfeff340247bc1fe28bf424)

# v1.3.0

## Fixes
- **Defensive min_scale clamp in navigation.c** - `world()` and `camera_update()` now floor `camera.scale` to `0.001f` before any division, protecting against zero scale even if the config clamp is removed (`src/navigation.c:5,11`).
- **Fixed int overflow in save_to_ppm** - `image->width * image->height` computed as `unsigned long` instead of `unsigned int` before loop, preventing wrap-around on large displays (`src/screenshot.c:124`).
- **Extracted shader header generation to scripts/gen_shaders.sh** - Makefile recipe now delegates to a standalone script for clarity (`Makefile:36`, `scripts/gen_shaders.sh`).

## What's new
- **Wired up ppm_save config option** - when `ppm_save = true` in config, saves a PPM screenshot to `~/.config/cboomer/screenshot.ppm` on startup (`src/main.c:555-561`, `src/config.c:55-61`).
- **Implemented ppm_save_path config option** - `ppm_save_path` in config file now controls where the PPM is saved instead of a hardcoded path; `$HOME` is expanded at load time (`src/config.c:62-64`, `src/main.c:554-565`).

- **Added install-deps.sh** - automatically detects distro and installs all build dependencies (`scripts/install-deps.sh`).
- **Added release.sh** - automates changelog commit-hash replacement, tagging, pushing, and GitHub release creation (`scripts/release.sh`).

commit: [2f00a47](https://github.com/DavidBalishyan/cboomer/commit/2f00a4773c741ec7e1bf9a45b90ab9888c386767)

# v1.3.1

## What's new
- **Typed config values** - values can be written as `"string"`, `'c'`, `42`, `3.14`, `true`/`false`; the parser infers the type from the literal form (`src/config.c:52-63`).
- **Inline comments in config** - `#` now works mid-line, not just at the start (`src/config.c:32-33`).
- **`~` expansion in ppm_save_path** - `ppm_save_path = ~/cap.ppm` now expands like `$HOME/...` (`src/config.c:76-86`).
- **More boolean aliases** - `yes`/`no` and `on`/`off` accepted alongside `true`/`false`/`1`/`0` (`src/config.c:68-73`).
- **Line numbers in config warnings** - unknown key warnings now say which line (`src/config.c:91`).
- **Colored output** - `Warning:` and `Error:` are yellow/red when stderr is a terminal (`src/config.h:9-27`, `src/config.c:9-27`).
- **Warning on unquoted strings** - `ppm_save_path` without quotes prints a warning (`src/config.c:84`).
- **Editor support** - vim syntax highlighting and emacs major mode for config files (`editors/vim/cboomer.vim`, `editors/emacs/cboomer-mode.el`).
- **default_shader config option** - pick the starting shader by name (`Normal`, `Sepia`, etc.) instead of always starting at Normal (`src/config.c`).
- **mirror config option** - start with mirror mode enabled (`src/config.c`).
- **flashlight_radius config option** - set the initial flashlight radius in pixels (`src/config.c`).
- **scroll_invert config option** - invert scroll-to-zoom direction (`src/config.c`).
- **ShaderMode enum moved to config.h** - `shader_names` is now exported so the config parser can match shader names.

commit: [eb7d62c](https://github.com/DavidBalishyan/cboomer/commit/eb7d62cec997f7f65df10e8537d21465a354f369)
# v1.4.0

- **On-screen display** - `osd` config / `o` key to show shader name, zoom %, and FPS rendered directly on screen using a bitmap font (`src/osd.c`, `src/osd.h`, `src/font8x8.h`).
- **Smooth reset** - `smooth_reset` config option; when `true`, pressing `0` animates the camera back to origin instead of snapping (`src/navigation.c`).
- **Configurable font** - `font` config option: specify a full path or just a font name (e.g. `"DejaVuSans"`) to auto-search system font directories; falls back to detecting known paths (`src/config.c`, `src/osd.c`).
- **OSD shader files** - OSD vertex and fragment shaders moved from hardcoded C strings to `src/shaders/osd_vert.glsl` and `src/shaders/osd_frag.glsl`, processed through `scripts/gen_shaders.sh` at build time (`src/osd.c`, `Makefile`).
- **GL error checks** - `glGetError` checks after every `glCompileShader`, `glLinkProgram`, `glGenTextures`, `glTexImage2D`, and `glBufferData` call in both `main.c` and `osd.c`.
- **OSD positioning fix** - truetype text now accounts for font ascent so it doesn't render too close to the top of the window (`src/osd.c`).
- **Config comment fixes** - `osd` entry now clarifies the 'o' key toggles regardless of the config value; `font` no longer misleadingly claims it has no effect when `osd` is off; default font example is now a font name instead of a full path (`src/config.c`).
- **Shader cycle key repeat** - holding `t` now throttles to one switch per 200ms instead of flooding through all shaders (`src/main.c`).
- **XRR memory leak** - `XRRFreeScreenConfigInfo` now frees the `XRRScreenConfiguration` obtained by `XRRGetScreenInfo` (`src/main.c`).
- **Windowed mode remembers position** - `--windowed` saves the window position to `$HOME/.config/cboomer/state` on exit and restores it on startup (`src/main.c`).
- **Save view to PPM** - press `s` to save the current rendered view (with shaders applied) to `cboomer_<timestamp>.ppm` via `glReadPixels`; configurable via `screenshot_dir` config key, defaults to `~/Pictures/Screenshots` (`src/main.c`, `src/config.c`, `src/config.h`).

commit: [39f1496](https://github.com/DavidBalishyan/cboomer/commit/39f14961a30260109247298ad073219f6688947d)

# v1.4.1

- **Experimental Ninja build support** - `scripts/experimental/generate.pl` and `scripts/experimental/generate.py` generate a `build.ninja` that produces the same binary as `make`. Ninja provides parallel builds, automatic header dependency tracking via `.d` files, and faster incremental rebuilds.
    - Usage: `perl scripts/experimental/generate.pl [--dev] [--live] [--mitshm] [--select]` then `ninja`
    - Feature flags work the same as Make variants
    - The primary build system remains Make; this is experimental
- **`scripts/shader.sh ls`** command to list all fragment shaders.
- **`screenshot_format` config option** - choose `"ppm"` or `"png"` for `s` key saves. PNG writer uses zlib to produce standard PNG files.
- **Keyboard panning** - arrow keys, vi-style `h`/`j`/`k`/`l`, and Emacs-style `Ctrl`+`f`/`b`/`n`/`p` pan the image with momentum.

commit: [8c27209](https://github.com/DavidBalishyan/cboomer/commit/8c2720968b69082a3d01d1ca56bc2b0499770c21)

# v1.4.2

- **Color picker on OSD** - when the on-screen display is visible, it now shows the RGB values and hex code of the pixel under the cursor (`Color: rgb(255, 128, 64) #FF8040`). Reads directly from the captured XImage data (unmodified by shaders) for accurate color picking.
- **Zoom presets** - keys `1` through `5` instantly jump to zoom levels 1x (100%), 2x (200%), 4x (400%), 8x (800%), and 16x (1600%). Uses the existing smooth animation system when `smooth_reset` is enabled.
- **Rotation** - press `Ctrl+[` to rotate 90 degrees counter-clockwise, `Ctrl+]` to rotate 90 degrees clockwise. Cycles through 0/90/180/270 degrees. Works by remapping texture coordinates per-vertex and re-uploading the VBO.

commit: [3f498b7](https://github.com/DavidBalishyan/cboomer/commit/3f498b7682a90811fd46987279fa63dd4efff26c)

# v1.5.0

- **Texture filtering toggle** - press `i` to switch between `GL_NEAREST` (pixel-perfect, default) and `GL_LINEAR` (bilinear interpolation) for the `GL_TEXTURE_MAG_FILTER`. When zoomed in close, linear filtering smooths the image instead of showing hard pixel blocks. Current state shown on the OSD when enabled.

commit: [2e2beff](https://github.com/DavidBalishyan/cboomer/commit/2e2beff1d3fe16f419fbcc2bb372fd2b11910954)

# v1.5.1

- **Test harness** - new `tests/` directory with a homegrown, dependency-free runner (`tests/test.h`); 38 unit tests covering config parsing (`tests/test_config.c`), vec2 math (`tests/test_la.c`), camera physics (`tests/test_navigation.c`), and byte-exact PPM/PNG encoding with CRC and zlib roundtrip checks (`tests/test_screenshot.c`). Run with `make test`; no display needed. See `tests/README.md`.
- **Colored test output** - test results print green `ok` / red `FAIL` when stdout is a terminal, plain when piped (same tty detection as config warnings).
- **CI workflow** - `.github/workflows/ci.yml` builds the project and runs the tests on every push to main and every pull request.
- **Moved `shader_names` to config.c** - the array was defined in `main.c` but declared and consumed in the config module; moving it lets test binaries link `config.o` without `main.o` (`src/config.c`, `src/main.c`).
- **Test header dependency in Makefile** - test objects rebuild when `tests/test.h` changes (`Makefile`).
- **Lint covers tests** - `scripts/lint.sh` now runs cppcheck/clang-tidy over `tests/*.c` too.
- **Build-variant matrix script** - `scripts/build-matrix.sh` compiles all seven meaningful `dev`/`live`/`mitshm`/`select` combinations to catch breakage in `#ifdef`'d code paths that plain `make` never compiles; runs as its own CI job.
- **Memory checking script** - `scripts/memcheck.sh` runs the unit tests under AddressSanitizer/UBSan and valgrind. Sanitized objects build to `build/asan` via a new `SANITIZE` Makefile hook so they never mix with normal objects; runs as its own CI job.
- **zlib in dependency manifests** - zlib has been linked (`-lz`) since PNG support in v1.4.1 but was missing from `scripts/install-deps.sh` and `.envforge.yaml`; both now install it (`zlib1g-dev`/`zlib-devel`/`zlib`/`zlib-dev` per distro).

commit: [e365a36](https://github.com/DavidBalishyan/cboomer/commit/e365a36de81cb89359451fbca6e136c00213b075)

# v1.5.2

- **Implemented issue [#2](https://github.com/DavidBalishyan/cboomer/issues/2)** - `find_font` corrupts the directory path mid-scan 
- **Implemented issue [#4](https://github.com/DavidBalishyan/cboomer/issues/4)** - `ppm_save_path` leaked on every config reload
- **Implemented issue [#3](https://github.com/DavidBalishyan/cboomer/issues/3)** - Division by zero if refresh rate is 0

commit: [d49c4a9](https://github.com/DavidBalishyan/cboomer/commit/d49c4a9bbcd687a830d6b8e340d5a241ee73d4fc)
