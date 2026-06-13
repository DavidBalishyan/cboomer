#define _POSIX_C_SOURCE 199309L
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
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

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

typedef struct {
    const char *path;
    const char *content;
} Shader;

#ifdef DEVELOPER
static Shader vertex_shader;
static Shader fragment_shader;

static void reload_shader(Shader *shader) {
    FILE *f = fopen(shader->path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    free((void*)shader->content);
    char *buf = malloc((size_t)len + 1);
    fread(buf, 1, (size_t)len, f);
    buf[len] = '\0';
    shader->content = buf;
    fclose(f);
}
#endif

static GLuint new_shader(Shader shader, GLenum kind) {
    GLuint result = glCreateShader(kind);
    const GLchar *src = shader.content;
    glShaderSource(result, 1, &src, NULL);
    glCompileShader(result);

    GLint success;
    glGetShaderiv(result, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(result, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "------------------------------\n");
        fprintf(stderr, "Error during shader compilation: %s. Log:\n", shader.path);
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

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
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
                 Vec2f window_size, Mouse mouse, Flashlight flashlight, int mirror) {
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
    XGrabPointer(display, root, 0,
                 ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
                 GrabModeAsync, GrabModeAsync,
                 root, cursor, CurrentTime);
    XGrabKeyboard(display, root, 0,
                  GrabModeAsync, GrabModeAsync,
                  CurrentTime);

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
    fprintf(stderr, "X ELEVEN ERROR: %s\n", error_message);
    return 0;
}

static void usage(void) {
    fprintf(stderr, "Usage: cboomer [OPTIONS]\n");
    fprintf(stderr, "  -d, --delay <seconds: float>  delay execution of the program by provided <seconds>\n");
    fprintf(stderr, "  -h, --help                    show this help and exit\n");
    fprintf(stderr, "      --new-config [filepath]   generate a new default config at [filepath]\n");
    fprintf(stderr, "  -c, --config <filepath>       use config at <filepath>\n");
    fprintf(stderr, "  -V, --version                 show the current version and exit\n");
    fprintf(stderr, "  -w, --windowed                windowed mode instead of fullscreen\n");
}

int main(int argc, char **argv) {
    const char *home = getenv("HOME");
    if (!home) home = ".";

    char boomer_dir[4096];
    snprintf(boomer_dir, sizeof(boomer_dir), "%s/.config/cboomer", home);

    char config_file[8192];
    snprintf(config_file, sizeof(config_file), "%s/config", boomer_dir);

    int windowed = 0;
    double delay_sec = 0.0;

    {
        int i = 1;
        while (i < argc) {
            const char *arg = argv[i];

            if (strcmp(arg, "-d") == 0 || strcmp(arg, "--delay") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "No value is provided for %s\n", arg);
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
                    snprintf(dir, sizeof(dir), "%s", new_config_path);
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
                        fprintf(stderr, "Disaster prevented\n");
                        return 1;
                    }
                }

                generate_default_config(new_config_path);
                printf("Generated config at %s\n", new_config_path);
                return 0;
            } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--config") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "No value is provided for %s\n", arg);
                    usage();
                    return 1;
                }
                snprintf(config_file, sizeof(config_file), "%s", argv[i + 1]);
                i += 2;
            } else {
                fprintf(stderr, "Unknown flag `%s`\n", arg);
                usage();
                return 1;
            }
        }
    }

    if (delay_sec > 0.0) {
        struct timespec ts = { (time_t)delay_sec, (long)((delay_sec - (time_t)delay_sec) * 1e9) };
        nanosleep(&ts, NULL);
    }

    Config config = DEFAULT_CONFIG;
    {
        FILE *check = fopen(config_file, "r");
        if (check) {
            fclose(check);
            config = load_config(config_file);
        } else {
            fprintf(stderr, "%s doesn't exist. Using default values.\n", config_file);
        }
    }

    printf("Using config: min_scale=%.2f scroll_speed=%.2f drag_friction=%.2f scale_friction=%.2f\n",
           config.min_scale, config.scroll_speed, config.drag_friction, config.scale_friction);

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open display\n");
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

    int screen = DefaultScreen(display);
    int glx_major, glx_minor;
    if (!glXQueryVersion(display, &glx_major, &glx_minor) ||
        (glx_major == 1 && glx_minor < 3) ||
        (glx_major < 1)) {
        fprintf(stderr, "Invalid GLX version. Expected >=1.3\n");
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
        fprintf(stderr, "No appropriate visual found\n");
        XCloseDisplay(display);
        return 1;
    }
    printf("Visual 0x%x selected\n", (unsigned int)vi->visualid);

    XWindowAttributes root_attrs;
    XGetWindowAttributes(display, DefaultRootWindow(display), &root_attrs);

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
        0, 0, root_attrs.width, root_attrs.height, 0,
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

#ifdef DEVELOPER
    vertex_shader.path = "src/vert.glsl";
    vertex_shader.content = VERT_SRC;
    fragment_shader.path = "src/frag.glsl";
    fragment_shader.content = FRAG_SRC;
#else
    Shader vertex_shader = { .path = "(embedded)", .content = VERT_SRC };
    Shader fragment_shader = { .path = "(embedded)", .content = FRAG_SRC };
#endif
    GLuint shader_program = new_shader_program(vertex_shader, fragment_shader);

    Screenshot screenshot = new_screenshot(display, tracking_window);

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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLsizei stride = (GLsizei)(4 * sizeof(float));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenshot.image->width, screenshot.image->height,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, screenshot.image->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);

    glEnable(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    int quitting = 0;
    Camera camera = { .scale = 1.0f };
    Vec2f cursor_pos = get_cursor_position(display);
    Mouse mouse = { .curr = cursor_pos, .prev = cursor_pos, .drag = 0 };
    Flashlight flashlight = { .is_enabled = 0, .radius = 200.0f, .shadow = 0.0f, .delta_radius = 0.0f };
    int mirror = 0;

    float dt = 1.0f / (float)rate;
    Window origin_window;
    int revert_to_return;
    XGetInputFocus(display, &origin_window, &revert_to_return);

    while (!quitting) {
        if (!windowed) {
            XSetInputFocus(display, win, RevertToParent, CurrentTime);
        }

        XWindowAttributes wa;
        XGetWindowAttributes(display, win, &wa);
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
                            camera.delta_scale += config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    } else if (key == XK_minus || key == XK_KP_Subtract) {
                        if (ctrl && flashlight.is_enabled) {
                            flashlight.delta_radius -= INITIAL_FL_DELTA_RADIUS;
                        } else {
                            camera.delta_scale -= config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    } else if (key == XK_0) {
                        camera.scale = 1.0f;
                        camera.delta_scale = 0.0;
                        camera.position = vec2(0.0f, 0.0f);
                        camera.velocity = vec2(0.0f, 0.0f);
                        mirror = 0;
                    } else if (key == XK_q || key == XK_Escape) {
                        quitting = 1;
                    } else if (key == XK_r) {
                        if (strlen(config_file) > 0) {
                            FILE *check = fopen(config_file, "r");
                            if (check) {
                                fclose(check);
                                config = load_config(config_file);
                            }
                        }

#ifdef DEVELOPER
                        if (ctrl) {
                            printf("------------------------------\n");
                            printf("RELOADING SHADERS\n");
                            reload_shader(&vertex_shader);
                            reload_shader(&fragment_shader);
                            GLuint new_program = new_shader_program(vertex_shader, fragment_shader);
                            glDeleteProgram(shader_program);
                            shader_program = new_program;
                            printf("Shader program ID: %u\n", shader_program);
                            printf("------------------------------\n");
                        }
#endif
                    } else if (key == XK_m) {
                        camera.position.x += (float)screenshot.image->width / camera.scale - 2.0f * (mouse.curr.x / camera.scale + camera.position.x);
                        mirror = !mirror;
                    } else if (key == XK_f) {
                        flashlight.is_enabled = !flashlight.is_enabled;
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
                            camera.delta_scale += config.scroll_speed;
                            camera.scale_pivot = mouse.curr;
                        }
                    } else if (xev.xbutton.button == Button5) {
                        if ((xev.xkey.state & ControlMask) && flashlight.is_enabled) {
                            flashlight.delta_radius -= INITIAL_FL_DELTA_RADIUS;
                        } else {
                            camera.delta_scale -= config.scroll_speed;
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

        draw(screenshot.image, camera, shader_program, vao, texture,
             vec2((float)wa.width, (float)wa.height),
             mouse, flashlight, mirror);

        glXSwapBuffers(display, win);
        glFinish();

#ifdef LIVE
        refresh_screenshot(&screenshot, display, tracking_window);
        {
            float nw = (float)screenshot.image->width;
            float nh = (float)screenshot.image->height;
            float new_vertices[] = {
                nw,   0.0f, 0.0f, 1.0f, 1.0f,
                nw,   nh,   0.0f, 1.0f, 0.0f,
                0.0f, nh,   0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f
            };
            GLuint new_indices[] = {0, 1, 3, 1, 2, 3};
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(new_vertices), new_vertices, GL_STATIC_DRAW);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenshot.image->width, screenshot.image->height,
                         0, GL_BGRA, GL_UNSIGNED_BYTE, screenshot.image->data);
        }
#endif
    }

    XSetInputFocus(display, origin_window, RevertToParent, CurrentTime);
    XSync(display, 0);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
    destroy_screenshot(screenshot, display);
    glXDestroyContext(display, glc);
    XDestroyWindow(display, win);
    XCloseDisplay(display);

    return 0;
}
