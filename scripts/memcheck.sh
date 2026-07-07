#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-all}"

if [ "$MODE" = "help" ] || [ "$MODE" = "--help" ] || [ "$MODE" = "-h" ]; then
    echo "Usage: $0 [asan|valgrind|all]"
    echo "Run the unit tests under memory checkers."
    echo ""
    echo "Modes:"
    echo "  asan      Rebuild the tests with AddressSanitizer + UBSan and run"
    echo "            them (objects go to build/asan, kept separate from"
    echo "            normal builds)"
    echo "  valgrind  Run the normally-built tests under valgrind"
    echo "  all       asan, then valgrind if installed (default)"
    exit 0
fi

run_asan() {
    echo "==> tests under AddressSanitizer + UndefinedBehaviorSanitizer"
    make test BUILD=build/asan \
        SANITIZE="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer"
}

run_valgrind() {
    echo "==> tests under valgrind"
    make build/run_tests
    valgrind --quiet --error-exitcode=1 --leak-check=full build/run_tests
}

case "$MODE" in
    asan)
        run_asan
        ;;
    valgrind)
        if ! command -v valgrind >/dev/null 2>&1; then
            echo "Error: valgrind not installed"
            exit 1
        fi
        run_valgrind
        ;;
    all)
        run_asan
        if command -v valgrind >/dev/null 2>&1; then
            run_valgrind
        else
            echo "valgrind not installed, skipping (e.g. sudo apt install valgrind)"
        fi
        ;;
    *)
        echo "Unknown mode: $MODE"
        echo "Run '$0 help' for usage."
        exit 1
        ;;
esac
