#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdio.h>
#include <stdbool.h>

typedef struct {
    double min_scale;
    double scroll_speed;
    double drag_friction;
    double scale_friction;
    bool ppm_save;
    char *ppm_save_path;
} Config;

extern const Config DEFAULT_CONFIG;

Config load_config(const char *file_path);
void generate_default_config(const char *file_path);

#endif // CONFIG_H_
