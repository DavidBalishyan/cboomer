#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

const Config DEFAULT_CONFIG = {
    .min_scale = 0.01,
    .scroll_speed = 1.5,
    .drag_friction = 6.0,
    .scale_friction = 4.0,
    .ppm_save_path = "$HOME/.config/cboomer/screenshot.ppm",
    .ppm_save = false,
};

Config load_config(const char *file_path) {
    Config config = DEFAULT_CONFIG;

    FILE *f = fopen(file_path, "r");
    if (!f) {
        return config;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;

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
              if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                    config.ppm_save = true;
              } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                    config.ppm_save = false;
              }
        } else if (strcmp(key, "ppm_save_path") == 0) {
            config.ppm_save_path = malloc(strlen(value) + 1);
            strcpy(config.ppm_save_path, value);
        }
        else {
            fprintf(stderr, "Warning: unknown config key `%s`\n", key);
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

void generate_default_config(const char *file_path) {
    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "Could not open %s for writing\n", file_path);
        exit(1);
    }
    fprintf(f, "min_scale = %.2f\n", DEFAULT_CONFIG.min_scale);
    fprintf(f, "scroll_speed = %.2f\n", DEFAULT_CONFIG.scroll_speed);
    fprintf(f, "drag_friction = %.2f\n", DEFAULT_CONFIG.drag_friction);
    fprintf(f, "scale_friction = %.2f\n", DEFAULT_CONFIG.scale_friction);
    fprintf(f, "ppm_save_path = %s\n", DEFAULT_CONFIG.ppm_save_path);
    fprintf(f, "ppm_save = %s\n", DEFAULT_CONFIG.ppm_save ? "true" : "false");
    fclose(f);
}
