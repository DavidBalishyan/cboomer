<!-- Todo list generated and controlled by todos.sh
For more information run: ./todos.sh help
-->
[x] crash on shader hot-reload free() on embedded string constant
[x] LIVE mode rendering is garbled vertex layout mismatch (5 floats vs stride of 4)
[x] heap overflow in reload_shader when ftell fails (returns -1 then cast to size_t)
[x] MITSHM XShmCreateImage/shmget/shmat all unchecked segfault on failure
[x] MITSHM fall back to XGetImage if shared mem setup fails
[x] XVisualInfo leak from glXChooseVisual never XFree'd
[x] division by zero via config min_scale = 0
[x] dangling pointer in refresh_screenshot when both XGetSubImage and XGetImage fail
[x] config parser calls exit(1) on unknown keys just warn and skip
[x] nanosleep not retried on EINTR
[x] XGetWindowAttributes returns unchecked for root window and event loop
[x] XGrabPointer/XGrabKeyboard unchecked in SELECT mode
[x] partial fread in reload_shader leaves garbage in shader source
[x] DEVELOPER mode leaks shader heap buffers on exit
[x] snprintf truncation undetected for config paths
[x] dead strlen(config_file) > 0 check
[x] new_indices unused in LIVE build
[x] clamp min_scale in navigation.c too defensive in case someone removes the config clamp
[x] check for division by zero in vec2_div_f / vec2_div or just assert
[x] save_to_ppm int overflow (width * height)
[x] PPM output never called from main kill it or wire it up
[x] extract shader header generation to scripts/gen_shaders.sh
[x] implement ppm_save_path config option and wire it up in main
[x] add install-deps.sh script to automate build dependency installation
[x] add release.sh script to automate tagging and GitHub releases
[x] add inline comment support (#) and line numbers in config parser warnings
[x] add ~ expansion and yes/no/on/off boolean values to config parser
[x] add typed config values ("string", 'c', 42, 3.14, true/false)
[x] output quoted strings in generated default config
[x] add colored output (yellow warnings, red errors) with tty detection
[x] warn on unquoted string values in config
[x] add vim syntax highlighting and emacs major mode for config files
[x] add default_shader, mirror, flashlight_radius, scroll_invert config options
[x] add osd config option, 'o' key toggle, on-screen display
[x] add smooth_reset config option for animated camera reset on 0
[x] add glGetError checks after shader compilation / texture upload
[x] check if XRRGetScreenInfo / XRRConfigCurrentRate needs XFree
[x] accept key repeat in shader cycling currently floods on held 't'
[x] make --windowed mode remember window position between runs
[x] add screenshot_format config option for PNG saves
[x] keyboard panning with arrows, vi keys (h/j/k/l), emacs keys (C-f/b/n/p)
[x] color picker on OSD - RGB + hex under cursor
[x] zoom presets (keys 1-5: 100%, 200%, 400%, 800%, 1600%)
[x] rotation (Ctrl+[/]: CCW/CW 90 degrees)
[] **Wayland backend** full port needed see notes below
[] add a --monitor flag to pick which display to capture
[] refresh_screenshot MITSHM path if new_screenshot fails after destroy image is NULL
[] config reload on SIGHUP
[] maybe switch from XGetImage to a portal-based capture for Wayland compat
[] screenshot capture ext-image-capture-source or wlr-screencopy
[] wl_surface + xdg_toplevel instead of XCreateWindow
