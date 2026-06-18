#define _POSIX_C_SOURCE 199309L
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "la.h"
#include "config.h"
#include "navigation.h"
#include "screenshot.h"
#include "shaders.h"
#include "osd.h"

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

typedef struct {
    const char *path;
    const char *content;
} Shader;

const char *shader_names[SHADER_COUNT] = {
    "Normal",
    "Invert",
    "CRT",
    "Grayscale",
    "Edge",
    "VHS Glitch",
    "Distortion",
    "Zoom Blur",
    "Posterize",
    "Pixelate",
    "Sepia",
    "Emboss",
};

static Shader vertex_shader = { .path = "(embedded)", .content = VERT_SRC };
static Shader fragment_shaders[SHADER_COUNT] = {
    { .path = "(embedded)", .content = FRAG_SRC },
    { .path = "(embedded)", .content = FRAG_INVERT_SRC },
    { .path = "(embedded)", .content = FRAG_CRT_SRC },
    { .path = "(embedded)", .content = FRAG_GRAYSCALE_SRC },
    { .path = "(embedded)", .content = FRAG_EDGE_SRC },
    { .path = "(embedded)", .content = FRAG_VHSGLITCH_SRC },
    { .path = "(embedded)", .content = FRAG_DISTORTION_SRC },
    { .path = "(embedded)", .content = FRAG_ZOOMBLUR_SRC },
    { .path = "(embedded)", .content = FRAG_POSTERIZE_SRC },
    { .path = "(embedded)", .content = FRAG_PIXELATE_SRC },
    { .path = "(embedded)", .content = FRAG_SEPIA_SRC },
    { .path = "(embedded)", .content = FRAG_EMBOSS_SRC },
};
static GLuint programs[SHADER_COUNT];
static ShaderMode current_shader;

#ifdef DEVELOPER
static const char *fragment_paths[SHADER_COUNT] = {
    "src/shaders/frag.glsl",
    "src/shaders/frag_invert.glsl",
    "src/shaders/frag_crt.glsl",
    "src/shaders/frag_grayscale.glsl",
    "src/shaders/frag_edge.glsl",
    "src/shaders/frag_vhsglitch.glsl",
    "src/shaders/frag_distortion.glsl",
    "src/shaders/frag_zoomblur.glsl",
    "src/shaders/frag_posterize.glsl",
    "src/shaders/frag_pixelate.glsl",
    "src/shaders/frag_sepia.glsl",
    "src/shaders/frag_emboss.glsl",
};

static void reload_shader(Shader *shader) {
    FILE *f = fopen(shader->path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0) {
        fclose(f);
        return;
    }
    free((void*)shader->content);
    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        shader->content = NULL;
        fclose(f);
        return;
    }
    size_t bytes_read = fread(buf, 1, (size_t)len, f);
    buf[bytes_read] = '\0';
    shader->content = buf;
    fclose(f);
}
#endif

static void state_save_pos(const char *dir, Display *display, Window win) {
    XWindowAttributes wa;
    if (!XGetWindowAttributes(display, win, &wa)) return;
    char path[8192];
    snprintf(path, sizeof(path), "%s/state", dir);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "win_x = %d\nwin_y = %d\n", wa.x, wa.y);
    fclose(f);
}

static int state_load_pos(const char *dir, int *x, int *y) {
    char path[8192];
    snprintf(path, sizeof(path), "%s/state", dir);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int got = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        int val;
        if (sscanf(line, "win_x = %d", &val) == 1) { *x = val; got |= 1; }
        if (sscanf(line, "win_y = %d", &val) == 1) { *y = val; got |= 2; }
    }
    fclose(f);
    return got == 3;
}

static void save_view_to_ppm(int w, int h, const char *dir) {
    char expanded[1024];
    if (dir[0] == '~' && (dir[1] == '/' || dir[1] == '\0')) {
        const char *home = getenv("HOME");
        if (!home) home = ".";
        snprintf(expanded, sizeof(expanded), "%s%s", home, dir + 1);
        dir = expanded;
    }

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char fname[256];
    strftime(fname, sizeof(fname), "cboomer_%Y%m%d_%H%M%S.ppm", tm);

    mkdir(dir, 0755);

    char path[2048];
    snprintf(path, sizeof(path), "%s/%s", dir, fname);

    unsigned char *pixels = malloc((size_t)w * h * 3);
    if (!pixels) {
        err("Failed to allocate memory for view save\n");
        return;
    }
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    FILE *f = fopen(path, "wb");
    if (!f) {
        err("Failed to open %s for writing\n", path);
        free(pixels);
        return;
    }

    fprintf(f, "P6\n%d %d\n255\n", w, h);
    size_t row_bytes = (size_t)w * 3;
    for (int y = h - 1; y >= 0; y--) {
        fwrite(pixels + (size_t)y * row_bytes, 1, row_bytes, f);
    }

    fclose(f);
    free(pixels);
    printf("Saved view to %s\n", path);
}

static void gl_check_error(const char *where) {
    GLenum gle;
    while ((gle = glGetError()) != GL_NO_ERROR) {
        err("GL error 0x%x after %s\n", gle, where);
    }
}

static GLuint new_shader(Shader shader, GLenum kind) {
    GLuint result = glCreateShader(kind);
    const GLchar *src = shader.content;
    glShaderSource(result, 1, &src, NULL);
    glCompileShader(result);
    gl_check_error("shader compile");

    GLint success;
    glGetShaderiv(result, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(result, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "------------------------------\n");
        err("during shader compilation: %s. Log:\n", shader.path);
        fprintf(stderr, "%s\n", info_log);
        fprintf(stderr, "------------------------------\n");
    }
    return result;
}

static GLuint new_shader_program(Shader vertex, Shader fragment) {
    GLuint program = glCreateProgram();
    GLuint vs = new_shader(vertex, GL_VERTEX_SHADER);
    GLuint fs = new_shader(fragment, GL_FRAGMENT_SHADER);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    gl_check_error("program link");

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
        err("shader program link failed\n");
        fprintf(stderr, "%s\n", info_log);
    }

    glUseProgram(program);
    return program;
}

typedef struct {
    int is_enabled;
    float shadow;
    float radius;
    float delta_radius;
} Flashlight;

#define INITIAL_FL_DELTA_RADIUS 250.0f
#define FL_DELTA_RADIUS_DECELERATION 10.0f

static void flashlight_update(Flashlight *fl, float dt) {
    if (fabsf(fl->delta_radius) > 1.0f) {
        fl->radius = fmaxf(0.0f, fl->radius + fl->delta_radius * dt);
        fl->delta_radius -= fl->delta_radius * FL_DELTA_RADIUS_DECELERATION * dt;
    }

    if (fl->is_enabled) {
        fl->shadow = fminf(fl->shadow + 6.0f * dt, 0.8f);
    } else {
        fl->shadow = fmaxf(fl->shadow - 6.0f * dt, 0.0f);
    }
}

static void draw(XImage *screenshot, Camera camera, GLuint shader, GLuint vao, GLuint texture,
                 Vec2f window_size, Mouse mouse, Flashlight flashlight, int mirror, float time) {
    (void)texture;
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader, "cameraPos"), camera.position.x, camera.position.y);
    glUniform1f(glGetUniformLocation(shader, "cameraScale"), camera.scale);
    glUniform2f(glGetUniformLocation(shader, "screenshotSize"), (float)screenshot->width, (float)screenshot->height);
    glUniform2f(glGetUniformLocation(shader, "windowSize"), window_size.x, window_size.y);
    glUniform2f(glGetUniformLocation(shader, "cursorPos"), mouse.curr.x, mouse.curr.y);
    glUniform1f(glGetUniformLocation(shader, "flShadow"), flashlight.shadow);
    glUniform1f(glGetUniformLocation(shader, "flRadius"), flashlight.radius);
    glUniform1i(glGetUniformLocation(shader, "mirror"), mirror ? 1 : 0);
    glUniform1f(glGetUniformLocation(shader, "time"), time);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

static Vec2f get_cursor_position(Display *display) {
    Window root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(display, DefaultRootWindow(display),
                  &root, &child,
                  &root_x, &root_y,
                  &win_x, &win_y,
                  &mask);
    return vec2((float)root_x, (float)root_y);
}

#ifdef SELECT
static Window select_window(Display *display) {
    Cursor cursor = XCreateFontCursor(display, XC_crosshair);
    Window root = DefaultRootWindow(display);
    if (XGrabPointer(display, root, 0,
                     ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync,
                     root, cursor, CurrentTime) != GrabSuccess) {
        XFreeCursor(display, cursor);
        return root;
    }
    if (XGrabKeyboard(display, root, 0,
                      GrabModeAsync, GrabModeAsync,
                      CurrentTime) != GrabSuccess) {
        XUngrabPointer(display, CurrentTime);
        XFreeCursor(display, cursor);
        return root;
    }

    XEvent event;
    Window result = root;
    while (1) {
        XNextEvent(display, &event);
        switch (event.type) {
            case ButtonPress:
                result = event.xbutton.subwindow;
                goto done;
            case KeyPress:
                result = root;
                goto done;
        }
    }
done:
    XUngrabPointer(display, CurrentTime);
    XUngrabKeyboard(display, CurrentTime);
    XFreeCursor(display, cursor);
    return result;
}
#endif

static int x_error_handler(Display *display, XErrorEvent *error) {
    char error_message[256];
    XGetErrorText(display, error->error_code, error_message, sizeof(error_message));
    err("X11: %s\n", error_message);
    return 0;
}

static void usage(void) {
    fprintf(stderr, "cboomer  -  fullscreen screenshot viewer\n\n");

    fprintf(stderr, "Usage:  cboomer [OPTIONS]\n\n");

    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d, --delay <seconds>    delay execution by <seconds>\n");
    fprintf(stderr, "  -h, --help               show this help and exit\n");
    fprintf(stderr, "      --new-config [path]  generate a default config at [path]\n");
    fprintf(stderr, "  -c, --config <path>      use config at <path>\n");
    fprintf(stderr, "  -V, --version            show version and exit\n");
    fprintf(stderr, "  -w, --windowed           windowed mode instead of fullscreen\n");

    fprintf(stderr, "\nControls:\n");
    fprintf(stderr, "  Mouse\n");
    fprintf(stderr, "    drag                   pan the image\n");
    fprintf(stderr, "    scroll                 zoom in / out\n");
    fprintf(stderr, "  Keys\n");
    fprintf(stderr, "    q / Esc                quit\n");
    fprintf(stderr, "    0                      reset position, scale, mirror\n");
    fprintf(stderr, "    = / -                  zoom in / out\n");
    fprintf(stderr, "    m                      mirror image horizontally\n");
    fprintf(stderr, "    f                      toggle flashlight\n");
    fprintf(stderr, "    t                      cycle shader modes\n");
    fprintf(stderr, "    o                      toggle on-screen display\n");
    fprintf(stderr, "    r                      reload config\n");
#ifdef DEVELOPER
    fprintf(stderr, "    Ctrl+r                 reload shaders from disk\n");
#endif
    fprintf(stderr, "    Ctrl+scroll/+/-        adjust flashlight radius\n");

#ifdef DEVELOPER
    fprintf(stderr, "\nBuild features:\n");
    fprintf(stderr, "  DEVELOPER               Ctrl+r hot-reloads shaders\n");
#endif
#ifdef LIVE
    fprintf(stderr, "  LIVE                    screenshot refreshes every frame\n");
#endif
#ifdef MITSHM
    fprintf(stderr, "  MITSHM                  MIT-SHM for faster captures\n");
#endif
#ifdef SELECT
    fprintf(stderr, "  SELECT                  click to select target window\n");
#endif
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    const char *home = getenv("HOME");
    if (!home) home = ".";

    char boomer_dir[4096];
    if (snprintf(boomer_dir, sizeof(boomer_dir), "%s/.config/cboomer", home) >= (int)sizeof(boomer_dir)) {
        err("HOME path too long\n");
        return 1;
    }

    char config_file[8192];
    if (snprintf(config_file, sizeof(config_file), "%s/config", boomer_dir) >= (int)sizeof(config_file)) {
        err("Config path too long\n");
        return 1;
    }

    int windowed = 0;
    double delay_sec = 0.0;

    {
        int i = 1;
        while (i < argc) {
            const char *arg = argv[i];

            if (strcmp(arg, "-d") == 0 || strcmp(arg, "--delay") == 0) {
                if (i + 1 >= argc) {
                    err("No value is provided for %s\n", arg);
                    usage();
                    return 1;
                }

                delay_sec = atof(argv[i + 1]);
                i += 2;
            } else if (strcmp(arg, "-w") == 0 || strcmp(arg, "--windowed") == 0) {
                windowed = 1;
                i += 1;
            } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
                usage();
                return 0;
            } else if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
                printf("cboomer-%s\n", GIT_HASH);
                return 0;
            } else if (strcmp(arg, "--new-config") == 0) {
                const char *new_config_path = NULL;
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    new_config_path = argv[i + 1];
                    i += 2;
                } else {
                    new_config_path = config_file;
                    i += 1;
                }

                {
                    char dir[8192];
                    if (snprintf(dir, sizeof(dir), "%s", new_config_path) >= (int)sizeof(dir)) {
                        err("Config path too long\n");
                        return 1;
                    }
                    char *slash = strrchr(dir, '/');
                    if (slash) {
                        *slash = '\0';
                        char *p = dir;
                        while (*p) {
                            p++;
                            if (*p == '/') {
                                *p = '\0';
                                mkdir(dir, 0755);
                                *p = '/';
                            }
                        }
                        mkdir(dir, 0755);
                    }
                }

                FILE *check = fopen(new_config_path, "r");
                if (check) {
                    fclose(check);
                    printf("File %s already exists. Replace it? [yn] ", new_config_path);
                    fflush(stdout);
                    char resp;
                    if (scanf("%c", &resp) == 1 && resp != 'y') {
                        err("Disaster prevented\n");
                        return 1;
                    }
                }

                generate_default_config(new_config_path);
                printf("Generated config at %s\n", new_config_path);
                return 0;
            } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--config") == 0) {
                if (i + 1 >= argc) {
                    err("No value is provided for %s\n", arg);
                    usage();
                    return 1;
                }
                if (snprintf(config_file, sizeof(config_file), "%s", argv[i + 1]) >= (int)sizeof(config_file)) {
                    err("Config path too long\n");
                    return 1;
                }
                i += 2;
            } else {
                err("Unknown flag `%s`\n", arg);
                usage();
                return 1;
            }
        }
    }

    if (delay_sec > 0.0) {
        struct timespec ts = { (time_t)delay_sec, (long)((delay_sec - (time_t)delay_sec) * 1e9) };
        while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
    }

    Config config = DEFAULT_CONFIG;
    {
        FILE *check = fopen(config_file, "r");
        if (check) {
            fclose(check);
            config = load_config(config_file);
        } else {
            warn("%s doesn't exist. Using default values.\n", config_file);
        }
    }

    current_shader = config.default_shader;

    int osd_enabled = config.osd;

    int win_x = 0, win_y = 0;
    if (windowed) {
        state_load_pos(boomer_dir, &win_x, &win_y);
    }

    printf("Using config: min_scale=%.2f scroll_speed=%.2f drag_friction=%.2f scale_friction=%.2f"
           " shader=%s mirror=%d osd=%d\n",
           config.min_scale, config.scroll_speed, config.drag_friction, config.scale_friction,
           shader_names[current_shader], config.mirror, osd_enabled);

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        err("Failed to open display\n");
        return 1;
    }

    XSetErrorHandler(x_error_handler);

#ifdef SELECT
    printf("Please select window:\n");
    Window tracking_window = select_window(display);
#else
    Window tracking_window = DefaultRootWindow(display);
#endif

    XRRScreenConfiguration *screen_config = XRRGetScreenInfo(display, DefaultRootWindow(display));
    int rate = XRRConfigCurrentRate(screen_config);
    printf("Screen rate: %d\n", rate);
    XRRFreeScreenConfigInfo(screen_config);

    int screen = DefaultScreen(display);
    int glx_major, glx_minor;
    if (!glXQueryVersion(display, &glx_major, &glx_minor) ||
        (glx_major == 1 && glx_minor < 3) ||
        (glx_major < 1)) {
        err("Invalid GLX version. Expected >=1.3\n");
        XCloseDisplay(display);
        return 1;
    }
    printf("GLX version %d.%d\n", glx_major, glx_minor);
    printf("GLX extension: %s\n", glXQueryExtensionsString(display, screen));

    int attrs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };

    XVisualInfo *vi = glXChooseVisual(display, 0, attrs);
    if (!vi) {
        err("No appropriate visual found\n");
        XCloseDisplay(display);
        return 1;
    }
    printf("Visual 0x%x selected\n", (unsigned int)vi->visualid);

    XWindowAttributes root_attrs;
    Status root_attrs_ok = XGetWindowAttributes(display, DefaultRootWindow(display), &root_attrs);
    if (!root_attrs_ok) {
        root_attrs.width = 1920;
        root_attrs.height = 1080;
    }

    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display, DefaultRootWindow(display), vi->visual, AllocNone);
    swa.event_mask = ButtonPressMask | ButtonReleaseMask |
                     KeyPressMask | KeyReleaseMask |
                     PointerMotionMask | ExposureMask | ClientMessage;
    if (!windowed) {
        swa.override_redirect = 1;
        swa.save_under = 1;
    }

    Window win = XCreateWindow(
        display, DefaultRootWindow(display),
        win_x, win_y, root_attrs.width, root_attrs.height, 0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder, &swa);

    XMapWindow(display, win);

    XClassHint hints;
    hints.res_name = "cboomer";
    hints.res_class = "Cboomer";
    XStoreName(display, win, "cboomer");
    XSetClassHint(display, win, &hints);

    Atom wm_delete_message = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, win, &wm_delete_message, 1);

    GLXContext glc = glXCreateContext(display, vi, NULL, GL_TRUE);
    glXMakeCurrent(display, win, glc);
    XFree(vi);

#ifdef DEVELOPER
    vertex_shader.path = "src/shaders/vert.glsl";
    for (int i = 0; i < SHADER_COUNT; i++) {
        fragment_shaders[i].path = fragment_paths[i];
    }
#endif
    for (int i = 0; i < SHADER_COUNT; i++) {
        programs[i] = new_shader_program(vertex_shader, fragment_shaders[i]);
    }
#ifdef DEVELOPER
    vertex_shader.content = NULL;
    for (int i = 0; i < SHADER_COUNT; i++) {
        fragment_shaders[i].content = NULL;
    }
#endif

    if (!osd_init(config.font)) {
        warn("OSD init failed\n");
    }

    Screenshot screenshot = new_screenshot(display, tracking_window);
    if (!screenshot.image) {
        err("Failed to capture screenshot\n");
        XDestroyWindow(display, win);
        XCloseDisplay(display);
        return 1;
    }

    if (config.ppm_save) {
        const char *path = config.ppm_save_path;
        char expanded[8192];
        const char *home = getenv("HOME");
        if (home && strncmp(path, "$HOME", 5) == 0) {
            if (snprintf(expanded, sizeof(expanded), "%s%s", home, path + 5) < (int)sizeof(expanded)) {
                path = expanded;
            }
        }
        save_to_ppm(screenshot.image, path);
        printf("Saved screenshot to %s\n", path);
    }

    float w = (float)screenshot.image->width;
    float h = (float)screenshot.image->height;

    GLuint vao, vbo, ebo;
    float vertices[] = {
        /* Position        Texture coords */
        w,    0.0f, 1.0f, 1.0f, /* Top right */
        w,    h,    1.0f, 0.0f, /* Bottom right */
        0.0f, h,    0.0f, 0.0f, /* Bottom left */
        0.0f, 0.0f, 0.0f, 1.0f  /* Top left */
    };
    GLuint indices[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    gl_check_error("vertex buffer upload");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    gl_check_error("index buffer upload");

    GLsizei stride = (GLsizei)(4 * sizeof(float));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint texture;
    glGenTextures(1, &texture);
    gl_check_error("texture generation");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenshot.image->width, screenshot.image->height,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, screenshot.image->data);
    gl_check_error("texture upload");
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(glGetUniformLocation(programs[current_shader], "tex"), 0);

    glEnable(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    int quitting = 0;
    Camera camera = { .scale = 1.0f };
    Vec2f cursor_pos = get_cursor_position(display);
    Mouse mouse = { .curr = cursor_pos, .prev = cursor_pos, .drag = 0 };
    Flashlight flashlight = { .is_enabled = 0, .radius = config.flashlight_radius, .shadow = 0.0f, .delta_radius = 0.0f };
    int mirror = config.mirror;
    int save_view = 0;
    float elapsed = 0.0f;
    float fps = (float)rate;

    float dt = 1.0f / (float)rate;
    struct timespec prev_ts;
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);
    Window origin_window;
    int revert_to_return;
    XGetInputFocus(display, &origin_window, &revert_to_return);

    while (!quitting) {
        if (!windowed) {
            XSetInputFocus(display, win, RevertToParent, CurrentTime);
        }

        XWindowAttributes wa;
        if (!XGetWindowAttributes(display, win, &wa)) continue;
        glViewport(0, 0, wa.width, wa.height);

        XEvent xev;
        while (XPending(display) > 0) {
            XNextEvent(display, &xev);

            switch (xev.type) {
                case Expose:
                    break;

                case MotionNotify:
                    mouse.curr = vec2((float)xev.xmotion.x, (float)xev.xmotion.y);
                    if (mouse.drag) {
                        Vec2f delta = vec2_sub(world(camera, mouse.prev), world(camera, mouse.curr));
                        camera.position = vec2_add(camera.position, delta);
                        camera.velocity = vec2_mul_f(delta, (float)rate);
                    }
                    mouse.prev = mouse.curr;
                    break;

                case ClientMessage:
                    if ((Atom)xev.xclient.data.l[0] == wm_delete_message) {
                        quitting = 1;
                    }
                    break;

                case KeyPress: {
                    KeySym key = XLookupKeysym((XKeyEvent*)&xev, 0);
                    int ctrl = (xev.xkey.state & ControlMask) != 0;

                    if (key == XK_equal || key == XK_KP_Add) {
                        if (ctrl && flashlight.is_enabled) {
                            flashlight.delta_radius += INITIAL_FL_DELTA_RADIUS;
                        } else {
                            camera.delta_scale += config.scroll_invert ? -config.scroll_speed : config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    } else if (key == XK_minus || key == XK_KP_Subtract) {
                        if (ctrl && flashlight.is_enabled) {
                            flashlight.delta_radius -= INITIAL_FL_DELTA_RADIUS;
                        } else {
                            camera.delta_scale -= config.scroll_invert ? -config.scroll_speed : config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    } else if (key == XK_0) {
                        if (config.smooth_reset) {
                            camera.anim_start_pos = camera.position;
                            camera.anim_start_scale = camera.scale;
                            camera.anim_end_pos = vec2(0.0f, 0.0f);
                            camera.anim_end_scale = 1.0f;
                            camera.anim_t = 0.0f;
                            camera.animating = 1;
                            camera.velocity = vec2(0.0f, 0.0f);
                            camera.delta_scale = 0.0;
                        } else {
                            camera.scale = 1.0f;
                            camera.delta_scale = 0.0;
                            camera.position = vec2(0.0f, 0.0f);
                            camera.velocity = vec2(0.0f, 0.0f);
                        }
                        mirror = 0;
                    } else if (key == XK_q || key == XK_Escape) {
                        quitting = 1;
                    } else if (key == XK_r) {
                        FILE *check = fopen(config_file, "r");
                        if (check) {
                            fclose(check);
                            config = load_config(config_file);
                            current_shader = config.default_shader;
                            mirror = config.mirror;
                            flashlight.radius = config.flashlight_radius;
                            printf("Config reloaded: shader=%s mirror=%d scroll_invert=%d osd=%d smooth_reset=%d\n",
                                   shader_names[current_shader], mirror, config.scroll_invert, config.osd, config.smooth_reset);
                        }

#ifdef DEVELOPER
                        if (ctrl) {
                            printf("------------------------------\n");
                            printf("RELOADING SHADERS\n");
                            reload_shader(&vertex_shader);
                            for (int i = 0; i < SHADER_COUNT; i++) {
                                reload_shader(&fragment_shaders[i]);
                            }
                            for (int i = 0; i < SHADER_COUNT; i++) {
                                GLuint new_program = new_shader_program(vertex_shader, fragment_shaders[i]);
                                glDeleteProgram(programs[i]);
                                programs[i] = new_program;
                            }
                            printf("Shader programs reloaded\n");
                            printf("------------------------------\n");
                        }
#endif
                    } else if (key == XK_m) {
                        camera.position.x += (float)screenshot.image->width / camera.scale - 2.0f * (mouse.curr.x / camera.scale + camera.position.x);
                        mirror = !mirror;
                    } else if (key == XK_o) {
                        osd_enabled = !osd_enabled;
                        printf("OSD: %s\n", osd_enabled ? "on" : "off");
                    } else if (key == XK_f) {
                        flashlight.is_enabled = !flashlight.is_enabled;
                    } else if (key == XK_t) {
                        static Time last_t = 0;
                        Time now = xev.xkey.time;
                        if (now - last_t > 200) {
                            current_shader = (current_shader + 1) % SHADER_COUNT;
                            printf("Shader: %s\n", shader_names[current_shader]);
                            last_t = now;
                        }
                    } else if (key == XK_s) {
                        save_view = 1;
                    }
                    break;
                }

                case ButtonPress:
                    if (xev.xbutton.button == Button1) {
                        mouse.prev = mouse.curr;
                        mouse.drag = 1;
                        camera.velocity = vec2(0.0f, 0.0f);
                    } else if (xev.xbutton.button == Button4) {
                        if ((xev.xkey.state & ControlMask) && flashlight.is_enabled) {
                            flashlight.delta_radius += INITIAL_FL_DELTA_RADIUS;
                        } else {
                            camera.delta_scale += config.scroll_invert ? -config.scroll_speed : config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    } else if (xev.xbutton.button == Button5) {
                        if ((xev.xkey.state & ControlMask) && flashlight.is_enabled) {
                            flashlight.delta_radius -= INITIAL_FL_DELTA_RADIUS;
                        } else {
                            camera.delta_scale -= config.scroll_invert ? -config.scroll_speed : config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    }
                    break;

                case ButtonRelease:
                    if (xev.xbutton.button == Button1) {
                        mouse.drag = 0;
                    }
                    break;
            }
        }

        camera_update(&camera, config, dt, mouse, screenshot.image,
                      vec2((float)wa.width, (float)wa.height));
        flashlight_update(&flashlight, dt);

        draw(screenshot.image, camera, programs[current_shader], vao, texture,
             vec2((float)wa.width, (float)wa.height),
             mouse, flashlight, mirror, elapsed);
        elapsed += dt;

        if (osd_enabled) {
            float zoom = camera.scale * 100.0f;
            osd_render(shader_names[current_shader], zoom, fps, vec2((float)wa.width, (float)wa.height));
        }

        if (save_view) {
            const char *sd = config.screenshot_dir;
            if (!sd[0]) sd = "~/Pictures/Screenshots";
            save_view_to_ppm(wa.width, wa.height, sd);
            save_view = 0;
        }

        glXSwapBuffers(display, win);
        glFinish();

        {
            struct timespec curr_ts;
            clock_gettime(CLOCK_MONOTONIC, &curr_ts);
            float frame_dt = (float)((curr_ts.tv_sec - prev_ts.tv_sec) + (curr_ts.tv_nsec - prev_ts.tv_nsec) / 1e9);
            if (frame_dt > 0.001f) {
                fps = fps * 0.9f + (1.0f / frame_dt) * 0.1f;
            }
            prev_ts = curr_ts;
        }

#ifdef LIVE
        refresh_screenshot(&screenshot, display, tracking_window);
        {
            float nw = (float)screenshot.image->width;
            float nh = (float)screenshot.image->height;
            float new_vertices[] = {
                nw,   0.0f, 1.0f, 1.0f,
                nw,   nh,   1.0f, 0.0f,
                0.0f, nh,   0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(new_vertices), new_vertices, GL_STATIC_DRAW);
            gl_check_error("LIVE vertex buffer upload");
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenshot.image->width, screenshot.image->height,
                         0, GL_BGRA, GL_UNSIGNED_BYTE, screenshot.image->data);
            gl_check_error("LIVE texture upload");
        }
#endif
    }

    XSetInputFocus(display, origin_window, RevertToParent, CurrentTime);
    XSync(display, 0);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    for (int i = 0; i < SHADER_COUNT; i++) {
        glDeleteProgram(programs[i]);
    }
#ifdef DEVELOPER
    free((void*)vertex_shader.content);
    for (int i = 0; i < SHADER_COUNT; i++) {
        free((void*)fragment_shaders[i].content);
    }
#endif
    destroy_screenshot(screenshot, display);
    osd_cleanup();
    if (windowed) {
        state_save_pos(boomer_dir, display, win);
    }
    glXDestroyContext(display, glc);
    XDestroyWindow(display, win);
    XCloseDisplay(display);

    return 0;
}
