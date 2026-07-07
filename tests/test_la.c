#include "test.h"
#include "la.h"

#define EPS 1e-6f

static void test_vec2_add(void) {
    Vec2f v = vec2_add(vec2(1.0f, 2.0f), vec2(3.0f, -4.0f));
    ASSERT_NEAR(v.x, 4.0f, EPS);
    ASSERT_NEAR(v.y, -2.0f, EPS);
}

static void test_vec2_sub(void) {
    Vec2f v = vec2_sub(vec2(1.0f, 2.0f), vec2(3.0f, -4.0f));
    ASSERT_NEAR(v.x, -2.0f, EPS);
    ASSERT_NEAR(v.y, 6.0f, EPS);
}

static void test_vec2_mul(void) {
    Vec2f v = vec2_mul(vec2(2.0f, 3.0f), vec2(4.0f, -5.0f));
    ASSERT_NEAR(v.x, 8.0f, EPS);
    ASSERT_NEAR(v.y, -15.0f, EPS);
}

static void test_vec2_div(void) {
    Vec2f v = vec2_div(vec2(8.0f, -15.0f), vec2(4.0f, 5.0f));
    ASSERT_NEAR(v.x, 2.0f, EPS);
    ASSERT_NEAR(v.y, -3.0f, EPS);
}

static void test_vec2_mul_f(void) {
    Vec2f v = vec2_mul_f(vec2(1.5f, -2.0f), 2.0f);
    ASSERT_NEAR(v.x, 3.0f, EPS);
    ASSERT_NEAR(v.y, -4.0f, EPS);
}

static void test_vec2_div_f(void) {
    Vec2f v = vec2_div_f(vec2(3.0f, -4.0f), 2.0f);
    ASSERT_NEAR(v.x, 1.5f, EPS);
    ASSERT_NEAR(v.y, -2.0f, EPS);
}

static void test_vec2_add_eq(void) {
    Vec2f v = vec2(1.0f, 2.0f);
    vec2_add_eq(&v, vec2(3.0f, -4.0f));
    ASSERT_NEAR(v.x, 4.0f, EPS);
    ASSERT_NEAR(v.y, -2.0f, EPS);
}

static void test_vec2_sub_eq(void) {
    Vec2f v = vec2(1.0f, 2.0f);
    vec2_sub_eq(&v, vec2(3.0f, -4.0f));
    ASSERT_NEAR(v.x, -2.0f, EPS);
    ASSERT_NEAR(v.y, 6.0f, EPS);
}

static void test_vec2_length(void) {
    ASSERT_NEAR(vec2_length(vec2(3.0f, 4.0f)), 5.0f, EPS);
    ASSERT_NEAR(vec2_length(vec2(0.0f, 0.0f)), 0.0f, EPS);
    ASSERT_NEAR(vec2_length(vec2(-3.0f, -4.0f)), 5.0f, EPS);
}

static void test_vec2_normalize(void) {
    Vec2f v = vec2_normalize(vec2(3.0f, 4.0f));
    ASSERT_NEAR(vec2_length(v), 1.0f, EPS);
    ASSERT_NEAR(v.x, 0.6f, EPS);
    ASSERT_NEAR(v.y, 0.8f, EPS);
}

static void test_vec2_normalize_zero(void) {
    Vec2f v = vec2_normalize(vec2(0.0f, 0.0f));
    ASSERT_NEAR(v.x, 0.0f, EPS);
    ASSERT_NEAR(v.y, 0.0f, EPS);
}

void run_la_tests(void) {
    RUN_TEST(test_vec2_add);
    RUN_TEST(test_vec2_sub);
    RUN_TEST(test_vec2_mul);
    RUN_TEST(test_vec2_div);
    RUN_TEST(test_vec2_mul_f);
    RUN_TEST(test_vec2_div_f);
    RUN_TEST(test_vec2_add_eq);
    RUN_TEST(test_vec2_sub_eq);
    RUN_TEST(test_vec2_length);
    RUN_TEST(test_vec2_normalize);
    RUN_TEST(test_vec2_normalize_zero);
}
