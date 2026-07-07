#ifndef TEST_H_
#define TEST_H_

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

extern int tests_run;
extern int tests_failed;
extern int checks_failed_in_test;

#define COLOR_GREEN (isatty(STDOUT_FILENO) ? "\033[32m" : "")
#define COLOR_RED   (isatty(STDOUT_FILENO) ? "\033[31m" : "")
#define COLOR_RESET (isatty(STDOUT_FILENO) ? "\033[0m" : "")

#define RUN_TEST(fn) do { \
    checks_failed_in_test = 0; \
    tests_run++; \
    fn(); \
    if (checks_failed_in_test) { \
        tests_failed++; \
        printf("%sFAIL%s  %s\n", COLOR_RED, COLOR_RESET, #fn); \
    } else { \
        printf("%sok%s    %s\n", COLOR_GREEN, COLOR_RESET, #fn); \
    } \
} while (0)

#define FAIL_AT() do { \
    checks_failed_in_test++; \
    printf("  %sassert failed%s at %s:%d\n", \
           COLOR_RED, COLOR_RESET, __FILE__, __LINE__); \
} while (0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        FAIL_AT(); \
        printf("    expected true: %s\n", #cond); \
        return; \
    } \
} while (0)

#define ASSERT_EQ_INT(actual, expected) do { \
    long long _a = (long long)(actual); \
    long long _e = (long long)(expected); \
    if (_a != _e) { \
        FAIL_AT(); \
        printf("    %s == %lld, expected %lld\n", #actual, _a, _e); \
        return; \
    } \
} while (0)

#define ASSERT_EQ_STR(actual, expected) do { \
    const char *_a = (actual); \
    const char *_e = (expected); \
    if (!_a || !_e || strcmp(_a, _e) != 0) { \
        FAIL_AT(); \
        printf("    %s == \"%s\", expected \"%s\"\n", #actual, \
               _a ? _a : "(null)", _e ? _e : "(null)"); \
        return; \
    } \
} while (0)

#define ASSERT_NEAR(actual, expected, eps) do { \
    double _a = (double)(actual); \
    double _e = (double)(expected); \
    if (fabs(_a - _e) > (double)(eps)) { \
        FAIL_AT(); \
        printf("    %s == %g, expected %g (eps %g)\n", #actual, _a, _e, (double)(eps)); \
        return; \
    } \
} while (0)

#define ASSERT_MEM_EQ(actual, expected, n) do { \
    const unsigned char *_a = (const unsigned char *)(actual); \
    const unsigned char *_e = (const unsigned char *)(expected); \
    size_t _n = (size_t)(n); \
    size_t _i; \
    for (_i = 0; _i < _n; _i++) { \
        if (_a[_i] != _e[_i]) break; \
    } \
    if (_i < _n) { \
        FAIL_AT(); \
        printf("    %s differs from %s at byte %zu: 0x%02x != 0x%02x\n", \
               #actual, #expected, _i, _a[_i], _e[_i]); \
        return; \
    } \
} while (0)

void run_la_tests(void);
void run_config_tests(void);
void run_navigation_tests(void);
void run_screenshot_tests(void);

#endif // TEST_H_
