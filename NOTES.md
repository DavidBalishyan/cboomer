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

