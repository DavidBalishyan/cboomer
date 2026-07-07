#include "test.h"

int tests_run = 0;
int tests_failed = 0;
int checks_failed_in_test = 0;

int main(void) {
    run_la_tests();
    run_config_tests();
    run_navigation_tests();
    run_screenshot_tests();

    printf("\n%s%d tests, %d failed%s\n",
           tests_failed ? COLOR_RED : COLOR_GREEN,
           tests_run, tests_failed, COLOR_RESET);
    return tests_failed ? 1 : 0;
}
