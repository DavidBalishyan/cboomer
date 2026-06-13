CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c11 -O2 -g
LDFLAGS = -lX11 -lXrandr -lXext -lGL -lm
PREFIX ?= /usr/local

BUILD = build
SRC = src/main.c src/la.c src/config.c src/navigation.c src/screenshot.c
OBJ = $(SRC:src/%.c=$(BUILD)/%.o)
BIN = $(shell pwd)/cboomer

GIT_HASH := $(shell git rev-parse HEAD 2>/dev/null || echo unknown)
CFLAGS += -DGIT_HASH=\"$(GIT_HASH)\" -I$(BUILD)

.PHONY: all clean dev live mitshm select install uninstall reinstall help

all: $(BIN)

FRAG_SHADER_NAMES = frag_invert frag_crt frag_grayscale frag_edge

$(BUILD)/shaders.h: src/shaders/vert.glsl src/shaders/frag.glsl \
                    $(FRAG_SHADER_NAMES:%=src/shaders/%.glsl) | $(BUILD)
	{ \
		printf '#ifndef SHADERS_H_\n#define SHADERS_H_\n\n'; \
		printf 'static const char VERT_SRC[] = "'; \
		awk '{printf "%s\\n", $$0}' src/shaders/vert.glsl; \
		printf '";\n\n'; \
		printf 'static const char FRAG_SRC[] = "'; \
		awk '{printf "%s\\n", $$0}' src/shaders/frag.glsl; \
		printf '";\n\n'; \
		for f in $(FRAG_SHADER_NAMES); do \
			n=$$(echo "$$f" | sed 's/^frag_//' | tr '[:lower:]' '[:upper:]'); \
			printf "static const char FRAG_%s_SRC[] = \"" "$$n"; \
			awk '{printf "%s\\n", $$0}' "src/shaders/$$f.glsl"; \
			printf '";\n\n'; \
		done; \
		printf '#endif\n'; \
	} > $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/main.o: $(BUILD)/shaders.h

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

dev: CFLAGS += -DDEVELOPER
dev: clean cboomer

live: CFLAGS += -DLIVE
live: clean cboomer

mitshm: CFLAGS += -DMITSHM
mitshm: clean cboomer

select: CFLAGS += -DSELECT
select: clean cboomer

install: cboomer
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
	@echo "  clean      Remove build artifacts"
	@echo "  help       Show this message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)     Installation prefix"
	@echo "  DESTDIR=             Staging directory (for packaging)"
	@echo "  CC=$(CC)             C compiler"
	@echo "  CFLAGS=$(CFLAGS)"
	@echo ""
	@echo "Flags can be combined, e.g. 'make dev live mitshm select'"

clean:
	rm -rf $(BUILD) cboomer
