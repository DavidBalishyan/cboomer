#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <unistd.h>
#include "test.h"
#include "config.h"

#define TEST_HOME "/tmp/cbtesthome"

static char tmp_path[64];

static const char *write_tmp_config(const char *contents) {
    strcpy(tmp_path, "/tmp/cboomer_test_XXXXXX");
    int fd = mkstemp(tmp_path);
    if (fd == -1) {
        perror("mkstemp");
        exit(1);
    }
    FILE *f = fdopen(fd, "w");
    if (!f) {
        perror("fdopen");
        exit(1);
    }
    fputs(contents, f);
    fclose(f);
    return tmp_path;
}

static void cleanup(Config *config) {
    unlink(tmp_path);
    free_config(config);
}

static void test_missing_file_returns_defaults(void) {
    Config config = load_config("/nonexistent/cboomer/config");
    ASSERT_NEAR(config.min_scale, DEFAULT_CONFIG.min_scale, 1e-9);
    ASSERT_NEAR(config.scroll_speed, DEFAULT_CONFIG.scroll_speed, 1e-9);
    ASSERT_EQ_INT(config.ppm_save, DEFAULT_CONFIG.ppm_save);
    ASSERT_EQ_INT(config.default_shader, SHADER_NORMAL);
    ASSERT_EQ_INT(config.smooth_reset, true);
    /* Documents current behavior: the early return on a missing file skips
     * $HOME expansion, so the path stays as the unexpanded literal. */
    ASSERT_TRUE(config.ppm_save_path == DEFAULT_CONFIG.ppm_save_path);
    ASSERT_EQ_STR(config.ppm_save_path, "$HOME/.config/cboomer/screenshot.ppm");
}

static void test_empty_file_expands_default_path(void) {
    Config config = load_config(write_tmp_config(""));
    ASSERT_EQ_STR(config.ppm_save_path, TEST_HOME "/.config/cboomer/screenshot.ppm");
    ASSERT_NEAR(config.min_scale, DEFAULT_CONFIG.min_scale, 1e-9);
    cleanup(&config);
}

static void test_numeric_values(void) {
    Config config = load_config(write_tmp_config(
        "min_scale = 0.5\n"
        "scroll_speed = 2.5\n"
        "drag_friction = 1.25\n"
        "scale_friction = 8.0\n"
        "flashlight_radius = 123.5\n"));
    ASSERT_NEAR(config.min_scale, 0.5, 1e-9);
    ASSERT_NEAR(config.scroll_speed, 2.5, 1e-9);
    ASSERT_NEAR(config.drag_friction, 1.25, 1e-9);
    ASSERT_NEAR(config.scale_friction, 8.0, 1e-9);
    ASSERT_NEAR(config.flashlight_radius, 123.5f, 1e-6);
    cleanup(&config);
}

static void test_clamping(void) {
    Config config = load_config(write_tmp_config(
        "min_scale = 0.0001\n"
        "flashlight_radius = -5\n"));
    ASSERT_NEAR(config.min_scale, 0.001, 1e-9);
    ASSERT_NEAR(config.flashlight_radius, 0.0f, 1e-9);
    cleanup(&config);
}

static void test_bool_true_forms(void) {
    static const char *lines[] = {
        "ppm_save = true\n", "ppm_save = 1\n",
        "ppm_save = yes\n", "ppm_save = on\n",
    };
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        Config config = load_config(write_tmp_config(lines[i]));
        int saved = config.ppm_save;
        cleanup(&config);
        ASSERT_EQ_INT(saved, true);
    }
}

static void test_bool_false_forms(void) {
    static const char *lines[] = {
        "smooth_reset = false\n", "smooth_reset = 0\n",
        "smooth_reset = no\n", "smooth_reset = off\n",
    };
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        Config config = load_config(write_tmp_config(lines[i]));
        int saved = config.smooth_reset;
        cleanup(&config);
        ASSERT_EQ_INT(saved, false);
    }
}

static void test_bool_invalid_keeps_default(void) {
    Config config = load_config(write_tmp_config(
        "ppm_save = maybe\n"
        "smooth_reset = maybe\n"));
    ASSERT_EQ_INT(config.ppm_save, false);
    ASSERT_EQ_INT(config.smooth_reset, true);
    cleanup(&config);
}

static void test_default_shader(void) {
    Config config = load_config(write_tmp_config("default_shader = \"Sepia\"\n"));
    ASSERT_EQ_INT(config.default_shader, SHADER_SEPIA);
    cleanup(&config);
}

static void test_default_shader_unknown_keeps_default(void) {
    Config config = load_config(write_tmp_config("default_shader = \"Nope\"\n"));
    ASSERT_EQ_INT(config.default_shader, SHADER_NORMAL);
    cleanup(&config);
}

static void test_comments_and_whitespace(void) {
    Config config = load_config(write_tmp_config(
        "# full-line comment\n"
        "\n"
        "   min_scale   =   0.5   \n"
        "scroll_speed = 3.0 # inline comment\n"
        "\t# indented comment\n"));
    ASSERT_NEAR(config.min_scale, 0.5, 1e-9);
    ASSERT_NEAR(config.scroll_speed, 3.0, 1e-9);
    cleanup(&config);
}

static void test_malformed_lines_are_skipped(void) {
    Config config = load_config(write_tmp_config(
        "this line has no equals sign\n"
        "unknown_key = 42\n"
        "min_scale = 0.5\n"));
    ASSERT_NEAR(config.min_scale, 0.5, 1e-9);
    cleanup(&config);
}

static void test_string_quoting(void) {
    Config config = load_config(write_tmp_config(
        "font = \"DejaVuSans\"\n"
        "screenshot_dir = '/tmp/shots'\n"
        "screenshot_format = png\n"));  /* unquoted: warns but parses */
    ASSERT_EQ_STR(config.font, "DejaVuSans");
    ASSERT_EQ_STR(config.screenshot_dir, "/tmp/shots");
    ASSERT_EQ_STR(config.screenshot_format, "png");
    cleanup(&config);
}

static void test_ppm_save_path_home_var_expansion(void) {
    Config config = load_config(write_tmp_config(
        "ppm_save_path = \"$HOME/shot.ppm\"\n"));
    ASSERT_EQ_STR(config.ppm_save_path, TEST_HOME "/shot.ppm");
    cleanup(&config);
}

static void test_ppm_save_path_tilde_expansion(void) {
    Config config = load_config(write_tmp_config(
        "ppm_save_path = \"~/shot.ppm\"\n"));
    ASSERT_EQ_STR(config.ppm_save_path, TEST_HOME "/shot.ppm");
    cleanup(&config);
}

static void test_generate_and_load_roundtrip(void) {
    strcpy(tmp_path, "/tmp/cboomer_test_XXXXXX");
    int fd = mkstemp(tmp_path);
    ASSERT_TRUE(fd != -1);
    close(fd);
    generate_default_config(tmp_path);

    Config config = load_config(tmp_path);
    ASSERT_NEAR(config.min_scale, DEFAULT_CONFIG.min_scale, 1e-9);
    ASSERT_NEAR(config.scroll_speed, DEFAULT_CONFIG.scroll_speed, 1e-9);
    ASSERT_NEAR(config.drag_friction, DEFAULT_CONFIG.drag_friction, 1e-9);
    ASSERT_NEAR(config.scale_friction, DEFAULT_CONFIG.scale_friction, 1e-9);
    ASSERT_EQ_INT(config.ppm_save, DEFAULT_CONFIG.ppm_save);
    ASSERT_EQ_INT(config.default_shader, DEFAULT_CONFIG.default_shader);
    ASSERT_EQ_INT(config.mirror, DEFAULT_CONFIG.mirror);
    ASSERT_EQ_INT(config.scroll_invert, DEFAULT_CONFIG.scroll_invert);
    ASSERT_EQ_INT(config.osd, DEFAULT_CONFIG.osd);
    ASSERT_EQ_INT(config.smooth_reset, DEFAULT_CONFIG.smooth_reset);
    ASSERT_NEAR(config.flashlight_radius, DEFAULT_CONFIG.flashlight_radius, 1e-6);
    ASSERT_EQ_STR(config.screenshot_format, DEFAULT_CONFIG.screenshot_format);
    ASSERT_EQ_STR(config.ppm_save_path, TEST_HOME "/.config/cboomer/screenshot.ppm");
    cleanup(&config);
}

void run_config_tests(void) {
    setenv("HOME", TEST_HOME, 1);

    RUN_TEST(test_missing_file_returns_defaults);
    RUN_TEST(test_empty_file_expands_default_path);
    RUN_TEST(test_numeric_values);
    RUN_TEST(test_clamping);
    RUN_TEST(test_bool_true_forms);
    RUN_TEST(test_bool_false_forms);
    RUN_TEST(test_bool_invalid_keeps_default);
    RUN_TEST(test_default_shader);
    RUN_TEST(test_default_shader_unknown_keeps_default);
    RUN_TEST(test_comments_and_whitespace);
    RUN_TEST(test_malformed_lines_are_skipped);
    RUN_TEST(test_string_quoting);
    RUN_TEST(test_ppm_save_path_home_var_expansion);
    RUN_TEST(test_ppm_save_path_tilde_expansion);
    RUN_TEST(test_generate_and_load_roundtrip);
}
