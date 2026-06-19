#include <math.h>
#include "navigation.h"

Vec2f world(Camera camera, Vec2f v) {
    float s = fmaxf(camera.scale, 0.001f);
    return vec2_div_f(v, s);
}

Vec2f screen_to_screenshot(Vec2f screen_pos, Vec2f window_size, Vec2f screenshot_size, Camera camera) {
    float s = fmaxf(camera.scale, 0.001f);
    float ndc_x = screen_pos.x / window_size.x * 2.0f - 1.0f;
    float ndc_y = 1.0f - screen_pos.y / window_size.y * 2.0f;
    float ratio_x = window_size.x / screenshot_size.x / s;
    float ratio_y = window_size.y / screenshot_size.y / s;
    float v_x = (ndc_x * ratio_x + 1.0f) * 0.5f * screenshot_size.x;
    float v_y = (ndc_y * ratio_y + 1.0f) * 0.5f * screenshot_size.y;
    float sx = v_x + camera.position.x;
    float sy = screenshot_size.y - (v_y - camera.position.y);
    return vec2(sx, sy);
}

void camera_update(Camera *camera, Config config, float dt, Mouse mouse, XImage *image, Vec2f window_size) {
    (void)image;

    if (camera->animating) {
        camera->anim_t += dt * 2.0f;
        if (camera->anim_t >= 1.0f) {
            camera->anim_t = 1.0f;
            camera->animating = 0;
        }
        float t = camera->anim_t;
        float smooth = t * t * (3.0f - 2.0f * t);
        camera->position = vec2_add(camera->anim_start_pos,
            vec2_mul_f(vec2_sub(camera->anim_end_pos, camera->anim_start_pos), smooth));
        camera->scale = camera->anim_start_scale + (camera->anim_end_scale - camera->anim_start_scale) * smooth;
        camera->velocity = vec2(0.0f, 0.0f);
        camera->delta_scale = 0.0;
        return;
    }

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
