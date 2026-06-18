#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>

void warn(const char *fmt, ...);
void err(const char *fmt, ...);

typedef enum {
    SHADER_NORMAL,
    SHADER_INVERT,
    SHADER_CRT,
    SHADER_GRAYSCALE,
    SHADER_EDGE,
    SHADER_VHSGLITCH,
    SHADER_DISTORTION,
    SHADER_ZOOMBLUR,
    SHADER_POSTERIZE,
    SHADER_PIXELATE,
    SHADER_SEPIA,
    SHADER_EMBOSS,
    SHADER_COUNT,
} ShaderMode;

extern const char *shader_names[SHADER_COUNT];

typedef struct {
    double min_scale;
    double scroll_speed;
    double drag_friction;
    double scale_friction;
    bool ppm_save;
    char *ppm_save_path;
    int default_shader;
    bool mirror;
    float flashlight_radius;
    bool scroll_invert;
    bool osd;
    bool smooth_reset;
    char font[512];
    char screenshot_dir[512];
    char screenshot_format[8];
} Config;

extern const Config DEFAULT_CONFIG;

Config load_config(const char *file_path);
void generate_default_config(const char *file_path);

#endif // CONFIG_H_
