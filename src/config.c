#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

void warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (isatty(STDERR_FILENO))
        fprintf(stderr, "\033[33mWarning\033[0m: ");
    else
        fprintf(stderr, "Warning: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void err(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (isatty(STDERR_FILENO))
        fprintf(stderr, "\033[31mError\033[0m: ");
    else
        fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}

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

const Config DEFAULT_CONFIG = {
    .min_scale = 0.01,
    .scroll_speed = 1.5,
    .drag_friction = 6.0,
    .scale_friction = 4.0,
    .ppm_save_path = "$HOME/.config/cboomer/screenshot.ppm",
    .ppm_save = false,
    .default_shader = SHADER_NORMAL,
    .mirror = false,
    .flashlight_radius = 200.0f,
    .scroll_invert = false,
    .osd = false,
    .smooth_reset = true,
    .font = "",
    .screenshot_dir = "",
    .screenshot_format = "ppm",
};

Config load_config(const char *file_path) {
    Config config = DEFAULT_CONFIG;

    FILE *f = fopen(file_path, "r");
    if (!f) {
        return config;
    }

    char line[512];
    int lineno = 0;
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        char *hash = strchr(p, '#');
        if (hash) *hash = '\0';

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = p;
        char *value = eq + 1;

        while (*key == ' ' || *key == '\t') key++;
        char *end = key + strlen(key) - 1;
        while (end >= key && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
        *(end + 1) = '\0';

        while (*value == ' ' || *value == '\t') value++;
        end = value + strlen(value) - 1;
        while (end >= value && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
        *(end + 1) = '\0';

        int quoted = 0;
        if (*value == '"') {
            quoted = 1;
            value++;
            end = value + strlen(value) - 1;
            if (*end == '"') *end = '\0';
        } else if (*value == '\'') {
            quoted = 1;
            value++;
            end = value + strlen(value) - 1;
            if (*end == '\'') *end = '\0';
        }

        if (strcmp(key, "min_scale") == 0) {
            config.min_scale = atof(value);
            if (config.min_scale < 0.001) config.min_scale = 0.001;
        } else if (strcmp(key, "scroll_speed") == 0) {
            config.scroll_speed = atof(value);
        } else if (strcmp(key, "drag_friction") == 0) {
            config.drag_friction = atof(value);
        } else if (strcmp(key, "scale_friction") == 0) {
            config.scale_friction = atof(value);
        } else if (strcmp(key, "ppm_save") == 0) {
              if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                  strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                    config.ppm_save = true;
              } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                         strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
                    config.ppm_save = false;
              }
        } else if (strcmp(key, "default_shader") == 0) {
            if (!quoted) {
                warn("config line %d: string values should be quoted: `%s`\n", lineno, key);
            }
            for (int i = 0; i < SHADER_COUNT; i++) {
                if (strcmp(value, shader_names[i]) == 0) {
                    config.default_shader = i;
                    break;
                }
            }
        } else if (strcmp(key, "mirror") == 0) {
              if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                  strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                    config.mirror = true;
              } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                         strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
                    config.mirror = false;
              }
        } else if (strcmp(key, "osd") == 0) {
              if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                  strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                    config.osd = true;
              } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                         strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
                    config.osd = false;
              }
        } else if (strcmp(key, "smooth_reset") == 0) {
              if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                  strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                    config.smooth_reset = true;
              } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                         strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
                    config.smooth_reset = false;
              }
        } else if (strcmp(key, "font") == 0) {
            if (!quoted) {
                warn("config line %d: string values should be quoted: `%s`\n", lineno, key);
            }
            snprintf(config.font, sizeof(config.font), "%s", value);
        } else if (strcmp(key, "screenshot_dir") == 0) {
            if (!quoted) {
                warn("config line %d: string values should be quoted: `%s`\n", lineno, key);
            }
            snprintf(config.screenshot_dir, sizeof(config.screenshot_dir), "%s", value);
        } else if (strcmp(key, "screenshot_format") == 0) {
            if (!quoted) {
                warn("config line %d: string values should be quoted: `%s`\n", lineno, key);
            }
            snprintf(config.screenshot_format, sizeof(config.screenshot_format), "%s", value);
        } else if (strcmp(key, "scroll_invert") == 0) {
              if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
                  strcmp(value, "yes") == 0 || strcmp(value, "on") == 0) {
                    config.scroll_invert = true;
              } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
                         strcmp(value, "no") == 0 || strcmp(value, "off") == 0) {
                    config.scroll_invert = false;
              }
        } else if (strcmp(key, "flashlight_radius") == 0) {
            config.flashlight_radius = (float)atof(value);
            if (config.flashlight_radius < 0.0f) config.flashlight_radius = 0.0f;
        } else if (strcmp(key, "ppm_save_path") == 0) {
            if (!quoted) {
                warn("config line %d: string values should be quoted: `%s`\n", lineno, key);
            }
            config.ppm_save_path = malloc(strlen(value) + 1);
            strcpy(config.ppm_save_path, value);
            if (config.ppm_save_path[0] == '~' &&
                (config.ppm_save_path[1] == '/' || config.ppm_save_path[1] == '\0')) {
                char *home = getenv("HOME");
                if (home) {
                    char *expanded = malloc(strlen(home) + strlen(config.ppm_save_path + 1) + 1);
                    strcpy(expanded, home);
                    strcat(expanded, config.ppm_save_path + 1);
                    free(config.ppm_save_path);
                    config.ppm_save_path = expanded;
                }
            }
        }
        else {
            warn("config line %d: unknown key `%s`\n", lineno, key);
        }
    }

    fclose(f);

    char *home = getenv("HOME");
    if (home && config.ppm_save_path && strncmp(config.ppm_save_path, "$HOME", 5) == 0) {
        size_t len = strlen(home) + strlen(config.ppm_save_path + 5) + 1;
        char *expanded = malloc(len);
        snprintf(expanded, len, "%s%s", home, config.ppm_save_path + 5);
        if (config.ppm_save_path != DEFAULT_CONFIG.ppm_save_path) {
            free(config.ppm_save_path);
        }
        config.ppm_save_path = expanded;
    }

    return config;
}

void free_config(Config *config) {
    if (config->ppm_save_path != DEFAULT_CONFIG.ppm_save_path) {
        free(config->ppm_save_path);
        config->ppm_save_path = NULL;
    }
}

void generate_default_config(const char *file_path) {
    FILE *f = fopen(file_path, "w");
    if (!f) {
        err("Could not open %s for writing\n", file_path);
        exit(1);
    }
    fprintf(f, "# cboomer config\n");
    fprintf(f, "#\n");
    fprintf(f, "# This file is read from ~/.config/cboomer/config on startup.\n");
    fprintf(f, "# Run `cboomer --new-config` to regenerate.\n");
    fprintf(f, "#\n");
    fprintf(f, "# Types are inferred from the literal:\n");
    fprintf(f, "#   quoted \"string\", 'char', 42 (int), 3.14 (float), true/false yes/no on/off (bool)\n");
    fprintf(f, "# Comments start with #.\n");
    fprintf(f, "\n");
    fprintf(f, "# Minimum zoom scale (float). Prevents zooming out too far.\n");
    fprintf(f, "min_scale = %.2f\n\n", DEFAULT_CONFIG.min_scale);
    fprintf(f, "# Scroll/trackpad sensitivity (float). Higher = faster.\n");
    fprintf(f, "scroll_speed = %.2f\n\n", DEFAULT_CONFIG.scroll_speed);
    fprintf(f, "# Friction applied when dragging (float). 0 = no friction.\n");
    fprintf(f, "drag_friction = %.2f\n\n", DEFAULT_CONFIG.drag_friction);
    fprintf(f, "# Friction applied when zooming (float). 0 = no friction.\n");
    fprintf(f, "scale_friction = %.2f\n\n", DEFAULT_CONFIG.scale_friction);
    fprintf(f, "# Invert scroll-to-zoom direction (bool).\n");
    fprintf(f, "scroll_invert = %s\n\n", DEFAULT_CONFIG.scroll_invert ? "true" : "false");
    fprintf(f, "# Start with on-screen display visible (bool). Shows shader name, zoom level,\n");
    fprintf(f, "# and FPS. Can be toggled at any time with the 'o' key regardless of this value.\n");
    fprintf(f, "osd = %s\n\n", DEFAULT_CONFIG.osd ? "true" : "false");
    fprintf(f, "# Smooth animation when resetting with '0' key (bool).\n");
    fprintf(f, "smooth_reset = %s\n\n", DEFAULT_CONFIG.smooth_reset ? "true" : "false");
    fprintf(f, "# Path to a .ttf font file for the OSD (string). Can be a full path or just a\n");
    fprintf(f, "# font name (e.g. \"DejaVuSans\") to search the system. If not set, auto-detects.\n");
    fprintf(f, "# font = \"DejaVuSans\"\n\n");
    fprintf(f, "# Directory to save rendered view screenshots when pressing 's' (string).\n");
    fprintf(f, "# Supports ~/ expansion. Defaults to ~/Pictures/Screenshots if not set.\n");
    fprintf(f, "# screenshot_dir = \"~/Pictures/Screenshots\"\n\n");
    fprintf(f, "# Path to save a PPM screenshot on startup (string).\n");
    fprintf(f, "# Supports ~/ and $HOME/ expansion.\n");
    fprintf(f, "ppm_save_path = \"%s\"\n\n", DEFAULT_CONFIG.ppm_save_path);
    fprintf(f, "# Enable PPM screenshot on startup (bool). true/false, yes/no, on/off.\n");
    fprintf(f, "ppm_save = %s\n\n", DEFAULT_CONFIG.ppm_save ? "true" : "false");
    fprintf(f, "# Default shader (string). Matches one of the shader names shown when cycling with 't'.\n");
    fprintf(f, "# Options: Normal, Invert, CRT, Grayscale, Edge, VHS Glitch, Distortion,\n");
    fprintf(f, "#          Zoom Blur, Posterize, Pixelate, Sepia, Emboss\n");
    fprintf(f, "default_shader = \"%s\"\n\n", shader_names[DEFAULT_CONFIG.default_shader]);
    fprintf(f, "# Start with mirror mode enabled (bool).\n");
    fprintf(f, "mirror = %s\n\n", DEFAULT_CONFIG.mirror ? "true" : "false");
    fprintf(f, "# Initial flashlight radius in pixels (float).\n");
    fprintf(f, "flashlight_radius = %.1f\n\n", DEFAULT_CONFIG.flashlight_radius);
    fprintf(f, "# Image format for 's' key saves (string). \"ppm\" or \"png\".\n");
    fprintf(f, "screenshot_format = \"%s\"\n", DEFAULT_CONFIG.screenshot_format);
    fclose(f);
}
