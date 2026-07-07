#include "test.h"
#include "navigation.h"

#define EPS 1e-4f

static Camera zero_camera(void) {
    Camera camera;
    memset(&camera, 0, sizeof(camera));
    return camera;
}

static void test_world_divides_by_scale(void) {
    Camera camera = zero_camera();
    camera.scale = 2.0f;
    Vec2f v = world(camera, vec2(10.0f, -4.0f));
    ASSERT_NEAR(v.x, 5.0f, EPS);
    ASSERT_NEAR(v.y, -2.0f, EPS);
}

static void test_world_clamps_zero_scale(void) {
    Camera camera = zero_camera();
    camera.scale = 0.0f;
    Vec2f v = world(camera, vec2(1.0f, 0.0f));
    ASSERT_NEAR(v.x, 1.0f / 0.001f, 1.0f);
}

static void test_screen_to_screenshot_identity(void) {
    Camera camera = zero_camera();
    camera.scale = 1.0f;
    Vec2f size = vec2(800.0f, 600.0f);

    Vec2f center = screen_to_screenshot(vec2(400.0f, 300.0f), size, size, camera);
    ASSERT_NEAR(center.x, 400.0f, EPS);
    ASSERT_NEAR(center.y, 300.0f, EPS);

    Vec2f corner = screen_to_screenshot(vec2(0.0f, 0.0f), size, size, camera);
    ASSERT_NEAR(corner.x, 0.0f, EPS);
    ASSERT_NEAR(corner.y, 0.0f, EPS);
}

static void test_animation_completes(void) {
    Camera camera = zero_camera();
    camera.animating = 1;
    camera.anim_t = 0.0f;
    camera.anim_start_pos = vec2(100.0f, 50.0f);
    camera.anim_start_scale = 4.0f;
    camera.anim_end_pos = vec2(0.0f, 0.0f);
    camera.anim_end_scale = 1.0f;
    camera.velocity = vec2(500.0f, 0.0f);
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 0};

    camera_update(&camera, DEFAULT_CONFIG, 0.5f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_EQ_INT(camera.animating, 0);
    ASSERT_NEAR(camera.anim_t, 1.0f, EPS);
    ASSERT_NEAR(camera.position.x, 0.0f, EPS);
    ASSERT_NEAR(camera.position.y, 0.0f, EPS);
    ASSERT_NEAR(camera.scale, 1.0f, EPS);
    ASSERT_NEAR(camera.velocity.x, 0.0f, EPS);
    ASSERT_NEAR(camera.delta_scale, 0.0, EPS);
}

static void test_animation_smoothstep_midpoint(void) {
    Camera camera = zero_camera();
    camera.animating = 1;
    camera.anim_t = 0.0f;
    camera.anim_start_pos = vec2(0.0f, 0.0f);
    camera.anim_start_scale = 1.0f;
    camera.anim_end_pos = vec2(100.0f, 200.0f);
    camera.anim_end_scale = 3.0f;
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 0};

    /* dt 0.25 advances anim_t to 0.5; smoothstep(0.5) == 0.5 */
    camera_update(&camera, DEFAULT_CONFIG, 0.25f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_EQ_INT(camera.animating, 1);
    ASSERT_NEAR(camera.anim_t, 0.5f, EPS);
    ASSERT_NEAR(camera.position.x, 50.0f, EPS);
    ASSERT_NEAR(camera.position.y, 100.0f, EPS);
    ASSERT_NEAR(camera.scale, 2.0f, EPS);
}

static void test_zoom_inertia_at_center_pivot(void) {
    Camera camera = zero_camera();
    camera.scale = 1.0f;
    camera.delta_scale = 100.0;
    camera.scale_pivot = vec2(400.0f, 300.0f);
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 0};

    camera_update(&camera, DEFAULT_CONFIG, 0.1f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_NEAR(camera.scale, 11.0f, EPS);
    ASSERT_NEAR(camera.position.x, 0.0f, EPS);
    ASSERT_NEAR(camera.position.y, 0.0f, EPS);
    /* delta_scale decays by delta_scale * dt * scale_friction (4.0) */
    ASSERT_NEAR(camera.delta_scale, 60.0, 1e-3);
}

static void test_zoom_clamps_to_min_scale(void) {
    Camera camera = zero_camera();
    camera.scale = 1.0f;
    camera.delta_scale = -1000.0;
    camera.scale_pivot = vec2(400.0f, 300.0f);
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 0};

    camera_update(&camera, DEFAULT_CONFIG, 0.1f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_NEAR(camera.scale, (float)DEFAULT_CONFIG.min_scale, EPS);
}

static void test_drag_inertia(void) {
    Camera camera = zero_camera();
    camera.scale = 1.0f;
    camera.velocity = vec2(100.0f, 0.0f);
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 0};

    camera_update(&camera, DEFAULT_CONFIG, 0.1f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_NEAR(camera.position.x, 10.0f, EPS);
    /* velocity decays by velocity * dt * drag_friction (6.0) */
    ASSERT_NEAR(camera.velocity.x, 40.0f, 1e-3);
}

static void test_velocity_below_threshold_ignored(void) {
    Camera camera = zero_camera();
    camera.scale = 1.0f;
    camera.velocity = vec2(10.0f, 0.0f);  /* below VELOCITY_THRESHOLD (15) */
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 0};

    camera_update(&camera, DEFAULT_CONFIG, 0.1f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_NEAR(camera.position.x, 0.0f, EPS);
    ASSERT_NEAR(camera.velocity.x, 10.0f, EPS);
}

static void test_drag_suppresses_inertia(void) {
    Camera camera = zero_camera();
    camera.scale = 1.0f;
    camera.velocity = vec2(100.0f, 0.0f);
    Mouse mouse = {vec2(0, 0), vec2(0, 0), 1};

    camera_update(&camera, DEFAULT_CONFIG, 0.1f, mouse, NULL, vec2(800.0f, 600.0f));

    ASSERT_NEAR(camera.position.x, 0.0f, EPS);
    ASSERT_NEAR(camera.velocity.x, 100.0f, EPS);
}

void run_navigation_tests(void) {
    RUN_TEST(test_world_divides_by_scale);
    RUN_TEST(test_world_clamps_zero_scale);
    RUN_TEST(test_screen_to_screenshot_identity);
    RUN_TEST(test_animation_completes);
    RUN_TEST(test_animation_smoothstep_midpoint);
    RUN_TEST(test_zoom_inertia_at_center_pivot);
    RUN_TEST(test_zoom_clamps_to_min_scale);
    RUN_TEST(test_drag_inertia);
    RUN_TEST(test_velocity_below_threshold_ignored);
    RUN_TEST(test_drag_suppresses_inertia);
}
