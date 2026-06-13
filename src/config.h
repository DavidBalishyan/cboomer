#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdio.h>

typedef struct {
    double min_scale;
    double scroll_speed;
    double drag_friction;
    double scale_friction;
} Config;

extern const Config DEFAULT_CONFIG;

Config load_config(const char *file_path);
void generate_default_config(const char *file_path);

#endif
