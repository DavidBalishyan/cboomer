#include <math.h>
#include "navigation.h"

Vec2f world(Camera camera, Vec2f v) {
    float s = fmaxf(camera.scale, 0.001f);
    return vec2_div_f(v, s);
}

void camera_update(Camera *camera, Config config, float dt, Mouse mouse, XImage *image, Vec2f window_size) {
    (void)image;
    camera->scale = fmaxf(camera->scale, 0.001f);

    if (fabs(camera->delta_scale) > 0.5) {
        Vec2f p0 = vec2_div_f(vec2_sub(camera->scale_pivot, vec2_mul_f(window_size, 0.5f)), camera->scale);
        camera->scale = fmaxf(camera->scale + (float)(camera->delta_scale * dt), (float)config.min_scale);
        Vec2f p1 = vec2_div_f(vec2_sub(camera->scale_pivot, vec2_mul_f(window_size, 0.5f)), camera->scale);
        camera->position = vec2_add(camera->position, vec2_sub(p0, p1));
        camera->delta_scale -= camera->delta_scale * dt * config.scale_friction;
    }

    if (!mouse.drag && vec2_length(camera->velocity) > VELOCITY_THRESHOLD) {
        camera->position = vec2_add(camera->position, vec2_mul_f(camera->velocity, dt));
        camera->velocity = vec2_sub(camera->velocity, vec2_mul_f(camera->velocity, dt * (float)config.drag_friction));
    }
}
