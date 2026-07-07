CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c11 -O2 -g
LDLIBS = -lX11 -lXrandr -lXext -lGL -lm -lz
PREFIX ?= /usr/local

BUILD := build
SRC := src/main.c src/la.c src/config.c src/navigation.c src/screenshot.c src/osd.c
OBJ := $(SRC:src/%.c=$(BUILD)/%.o)
BIN := $(CURDIR)/cboomer

GIT_HASH := $(shell git rev-parse HEAD 2>/dev/null || echo unknown)
CFLAGS += -DGIT_HASH=\"$(GIT_HASH)\" -I$(BUILD)

# Sanitizer hook: scripts/memcheck.sh passes SANITIZE and a separate BUILD
# dir so instrumented objects never mix with normal ones
SANITIZE ?=
CFLAGS += $(SANITIZE)

ifneq ($(filter dev,$(MAKECMDGOALS)),)
CFLAGS += -DDEVELOPER
endif
ifneq ($(filter live,$(MAKECMDGOALS)),)
CFLAGS += -DLIVE
endif
ifneq ($(filter mitshm,$(MAKECMDGOALS)),)
CFLAGS += -DMITSHM
endif
ifneq ($(filter select,$(MAKECMDGOALS)),)
CFLAGS += -DSELECT
endif

.DELETE_ON_ERROR:
.PHONY: all clean dev live mitshm select install uninstall reinstall help test

all: $(BIN)

FRAG_SHADER_NAMES = frag_invert frag_crt frag_grayscale frag_edge frag_vhsglitch frag_distortion frag_zoomblur frag_posterize frag_pixelate frag_sepia frag_emboss

$(BUILD)/shaders.h: src/shaders/vert.glsl src/shaders/frag.glsl \
                    $(FRAG_SHADER_NAMES:%=src/shaders/%.glsl) \
                    src/shaders/osd_vert.glsl src/shaders/osd_frag.glsl | $(BUILD)
	scripts/gen_shaders.sh $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/main.o: $(BUILD)/shaders.h
$(BUILD)/osd.o: $(BUILD)/shaders.h

$(BUILD)/%.o: src/%.c Makefile | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

dev live mitshm select: $(BIN)

TEST_SRC := tests/test_main.c tests/test_la.c tests/test_config.c \
            tests/test_navigation.c tests/test_screenshot.c
TEST_OBJ := $(TEST_SRC:tests/%.c=$(BUILD)/tests/%.o)
TEST_DEPS := $(BUILD)/la.o $(BUILD)/config.o $(BUILD)/navigation.o $(BUILD)/screenshot.o
TEST_BIN := $(BUILD)/run_tests
# -lX11 only resolves symbols referenced by untested capture functions in
# screenshot.o; the test binary never opens a display.
TEST_LDLIBS := -lX11 -lm -lz

$(BUILD)/tests:
	mkdir -p $(BUILD)/tests

$(BUILD)/tests/%.o: tests/%.c tests/test.h Makefile | $(BUILD)/tests
	$(CC) $(CFLAGS) -Isrc -c -o $@ $<

$(TEST_BIN): $(TEST_OBJ) $(TEST_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(TEST_LDLIBS)

test: $(TEST_BIN)
	$(TEST_BIN)

install: $(BIN)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 cboomer $(DESTDIR)$(PREFIX)/bin/cboomer

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cboomer

reinstall: uninstall install

help:
	@echo "Usage: make [TARGET]"
	@echo ""
	@echo "Targets:"
	@echo "  all        Build cboomer (default)"
	@echo "  dev        Build with DEVELOPER (Ctrl+R shader hot-reload)"
	@echo "  live       Build with live screenshot refresh"
	@echo "  mitshm     Build with MIT-SHM (faster live updates)"
	@echo "  select     Build with window selection on startup"
	@echo "  install    Install cboomer to \$$(PREFIX)/bin"
	@echo "  uninstall  Remove cboomer from \$$(PREFIX)/bin"
	@echo "  reinstall  Uninstall then install"
	@echo "  test       Build and run the unit tests"
	@echo "  clean      Remove build artifacts"
	@echo "  help       Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)     Installation prefix"
	@echo "  DESTDIR=             Staging directory (for packaging)"
	@echo "  CC=$(CC)             C compiler"
	@echo "  CFLAGS=$(CFLAGS)"
	@echo ""
	@echo "Stack flags: 'make dev live' builds with -DDEVELOPER -DLIVE"
	@echo "Run 'make clean' between unrelated builds to avoid stale objects"

clean:
	rm -rf $(BUILD) cboomer
