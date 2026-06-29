#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "help" ]; then
    echo "Usage: $0"
    echo "Run static analysis on the cboomer source code."
    echo ""
    echo "Tries cppcheck first, then clang-tidy, then prints a"
    echo "message if neither is available."
    exit 0
fi

SRC="src/main.c src/la.c src/config.c src/navigation.c src/screenshot.c src/osd.c"

if command -v cppcheck >/dev/null 2>&1; then
    exec cppcheck --enable=all --suppress=missingIncludeSystem \
        --std=c11 --language=c $SRC
fi

if command -v clang-tidy >/dev/null 2>&1; then
    exec clang-tidy $SRC -- -std=c11 -Ibuild
fi

echo "Neither cppcheck nor clang-tidy found. Install one of them:"
echo "  Debian/Ubuntu: sudo apt install cppcheck"
echo "  Fedora:        sudo dnf install cppcheck"
echo "  Arch:          sudo pacman -S cppcheck"
echo "  Alpine:        sudo apk add cppcheck"
