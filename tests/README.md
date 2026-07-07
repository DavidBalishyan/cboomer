# Tests

Unit tests for the modules that don't need a display. The test binary never
opens an X connection, so the normal build dependencies are all you need.

## Running

```console
$ make test
```

Builds `build/run_tests` and runs it. The exit code is `0` when all tests
pass and non-zero otherwise. Output is colored when stdout is a terminal
(green `ok`, red `FAIL`) and plain when piped, e.g. in CI logs.

`scripts/memcheck.sh` runs the same tests under AddressSanitizer/UBSan
(instrumented objects go to `build/asan`) and valgrind. CI runs both.

## Layout

| File | Covers |
|---|---|
| `test.h` | Test runner: `RUN_TEST` plus the assert macros. Depends only on libc |
| `test_main.c` | Entry point; runs all suites and prints the summary |
| `test_la.c` | `src/la.c` - vec2 math (add/sub/mul/div, length, normalize) |
| `test_config.c` | `src/config.c` - config parsing, clamping, bools, comments, `$HOME`/`~` expansion, generate/load roundtrip |
| `test_navigation.c` | `src/navigation.c` - `world()`, `screen_to_screenshot()`, `camera_update()` physics (inertia, friction, smooth-reset animation, clamping) |
| `test_screenshot.c` | `src/screenshot.c` - byte-exact PPM output and full PNG verification (signature, IHDR, chunk CRCs, zlib IDAT roundtrip) |

Not covered: `src/main.c` and `src/osd.c`, which are X11/OpenGL glue that
needs a real display, and the capture functions in `src/screenshot.c`
(`new_screenshot`, `refresh_screenshot`).

## Writing a test

Tests are plain `static void` functions. Register one by calling it with
`RUN_TEST` inside the suite's `run_*_tests()` function; there is no other
registration step:

```c
#include "test.h"
#include "la.h"

static void test_vec2_add(void) {
    Vec2f v = vec2_add(vec2(1.0f, 2.0f), vec2(3.0f, -4.0f));
    ASSERT_NEAR(v.x, 4.0f, 1e-6f);
    ASSERT_NEAR(v.y, -2.0f, 1e-6f);
}

void run_la_tests(void) {
    RUN_TEST(test_vec2_add);
}
```

Every assert prints the file, line, and both values on failure, then
returns from the test:

| Macro | Use |
|---|---|
| `ASSERT_TRUE(cond)` | Any boolean condition |
| `ASSERT_EQ_INT(a, b)` | Integers, enums, bools |
| `ASSERT_EQ_STR(a, b)` | NUL-terminated strings (handles NULL) |
| `ASSERT_NEAR(a, b, eps)` | Floats/doubles within a tolerance |
| `ASSERT_MEM_EQ(a, b, n)` | Raw byte buffers; reports the first differing byte |

To add a new suite: create `tests/test_foo.c` with a `run_foo_tests()`
function, declare it in `test.h`, call it from `test_main.c`, and add the
file to `TEST_SRC` in the `Makefile` and to `SRC` in `scripts/lint.sh`.

## Notes

- Test objects link against the production objects (`la.o`, `config.o`,
  `navigation.o`, `screenshot.o`). `-lX11` is only there to resolve symbols
  referenced by the untested capture functions in `screenshot.o`; none of
  those functions ever run.
- Config tests `setenv("HOME", ...)` and write temp files via `mkstemp`.
  Screenshot tests build a fake `XImage` by hand, setting only `width`,
  `height`, and `data`. Never call `XDestroyImage` on it.
- The `Warning:` lines on stderr during the config suite are expected. They
  come from `warn()` in `src/config.c` while the tests exercise unknown keys
  and unquoted strings.
- Some tests pin current behavior rather than ideal behavior (e.g. a missing
  config file skips `$HOME` expansion). If you change that behavior on
  purpose, update the test; it exists to catch accidental changes.
- Float assertions on `camera_update` use an epsilon around `1e-4f` because
  it mixes double config fields with float math.
