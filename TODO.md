# TODO

## Done
- [x] crash on shader hot-reload free() on embedded string constant
- [x] LIVE mode rendering is garbled vertex layout mismatch (5 floats vs stride of 4)
- [x] heap overflow in reload_shader when ftell fails (returns -1 then cast to size_t)
- [x] MITSHM XShmCreateImage/shmget/shmat all unchecked segfault on failure
- [x] MITSHM fall back to XGetImage if shared mem setup fails
- [x] XVisualInfo leak from glXChooseVisual never XFree'd
- [x] division by zero via config min_scale = 0
- [x] dangling pointer in refresh_screenshot when both XGetSubImage and XGetImage fail
- [x] config parser calls exit(1) on unknown keys just warn and skip
- [x] nanosleep not retried on EINTR
- [x] XGetWindowAttributes returns unchecked for root window and event loop
- [x] XGrabPointer/XGrabKeyboard unchecked in SELECT mode
- [x] partial fread in reload_shader leaves garbage in shader source
- [x] DEVELOPER mode leaks shader heap buffers on exit
- [x] snprintf truncation undetected for config paths
- [x] dead strlen(config_file) > 0 check
- [x] new_indices unused in LIVE build
- [x] clamp min_scale in navigation.c too defensive in case someone removes the config clamp
- [x] check for division by zero in vec2_div_f / vec2_div or just assert
- [x] save_to_ppm int overflow (width * height)
- [x] PPM output never called from main kill it or wire it up
- [x] extract shader header generation to scripts/gen_shaders.sh
- [x] implement ppm_save_path config option and wire it up in main
- [x] add install-deps.sh script to automate build dependency installation
- [x] add release.sh script to automate tagging and GitHub releases
- [x] add inline comment support (#) and line numbers in config parser warnings
- [x] add ~ expansion and yes/no/on/off boolean values to config parser
- [x] add typed config values ("string", 'c', 42, 3.14, true/false)
- [x] output quoted strings in generated default config
- [x] add colored output (yellow warnings, red errors) with tty detection
- [x] warn on unquoted string values in config
- [x] add vim syntax highlighting and emacs major mode for config files
- [x] add default_shader, mirror, flashlight_radius, scroll_invert config options
- [x] add osd config option, 'o' key toggle, on-screen display
- [x] add smooth_reset config option for animated camera reset on 0
- [x] add glGetError checks after shader compilation / texture upload
- [x] check if XRRGetScreenInfo / XRRConfigCurrentRate needs XFree
- [x] accept key repeat in shader cycling currently floods on held 't'
- [x] make --windowed mode remember window position between runs
- [x] add screenshot_format config option for PNG saves
- [x] keyboard panning with arrows, vi keys (h/j/k/l), emacs keys (C-f/b/n/p)

## Done (v1.4.2)
- [x] color picker on OSD - RGB + hex under cursor
- [x] zoom presets (keys 1-5: 100%, 200%, 400%, 800%, 1600%)
- [x] rotation (Ctrl+[/]: CCW/CW 90 degrees)

## High priority

Clear

## Medium priority

Clear

## Low priority / icebox

- [ ] **Wayland backend** full port needed see notes below
- [ ] add a --monitor flag to pick which display to capture
- [ ] refresh_screenshot MITSHM path if new_screenshot fails after destroy image is NULL
- [ ] config reload on SIGHUP
- [ ] maybe switch from XGetImage to a portal-based capture for Wayland compat
- [ ] screenshot capture ext-image-capture-source or wlr-screencopy
- [ ] wl_surface + xdg_toplevel instead of XCreateWindow

>[!NOTE]
>These things probably aren't going to be implemented in the near future
## Wayland port notes

Biggest blockers:
1. No XGetImage need wlr-screencopy or ext-image-capture-source
2. No GLX need EGL + wayland-egl
3. No XQueryPointer pointer events come from wl_seat listeners
4. No XGrabPointer cant do the SELECT crosshair thing
5. No override_redirect xdg_toplevel.set_fullscreen() is a request not a command

Approach extract a platform abstraction layer (display/window/input/capture) keep all the GL rendering as-is. Compile-time backend selection with -DBACKEND_X11 or -DBACKEND_WAYLAND. See also wayland-ext-capture xdg-shell wayland-scanner for protocol stubs.

## Windows port notes

Same idea as Wayland a separate backend behind the same abstraction layer. Steps and gotchas:

1. Windowing RegisterClassEx then CreateWindowEx fullscreen via SetWindowLong + SetWindowPos no override_redirect here either
2. OpenGL ChoosePixelFormat SetPixelFormat then wglCreateContext instead of glXCreateContext the GL calls themselves stay the same
3. Screenshot capture no XGetImage or MITSHM here use GetDC BitBlt into a memory DC then GetDIBits to get the pixels out
4. Input no XNextEvent loop use PeekMessage/GetMessage map WM_KEYDOWN through MapVirtualKey mouse via WM_MOUSEMOVE using GET_X_LPARAM/GET_Y_LPARAM scroll via WM_MOUSEWHEEL
5. Fullscreen SetWindowLong with WS_EX_TOPMOST plus SetWindowPos no guarantee the compositor will refuse like on Wayland so this is actually easier
6. Config path %APPDATA%\cboomer instead of ~/.config/cboomer use getenv("APPDATA")
7. Timing no nanosleep use Sleep for delay dt from QueryPerformanceCounter instead of XRRConfigCurrentRate
8. Build system Makefile wont work need CMake or a simple batch script with MinGW flags like -lgdi32 -lopengl32 -lws2_32

Whats the same the entire GL rendering pipeline (shaders textures VAOs draw calls) and the camera/flashlight/input math. Only the platform glue changes.

Order of operations
1. Extract X11 code from main.c into src/platform_x11.c with a clean header src/platform.h
2. Write src/platform_win32.c behind the same header
3. Write src/platform_wayland.c behind the same header
4. main.c just calls platform_init() platform_poll() platform_swap() etc

SDL2/GLFW shortcut if we ever change our minds about staying low level that would give us all three platforms from one API but defeats the point of doing it manually
