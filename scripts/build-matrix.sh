#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "help" ]; then
    echo "Usage: $0"
    echo "Build cboomer with every meaningful variant combination to catch"
    echo "breakage in #ifdef'd code paths (DEVELOPER, LIVE, MITSHM, SELECT)."
    echo ""
    echo "Runs 'make clean' between builds and leaves the tree clean."
    exit 0
fi

COMBOS=(
    ""
    "dev"
    "live"
    "mitshm"
    "select"
    "live mitshm"
    "dev live mitshm select"
)

JOBS="$(nproc 2>/dev/null || echo 2)"

for combo in "${COMBOS[@]}"; do
    label="${combo:-release}"
    echo "==> building: $label"
    make clean > /dev/null
    # $combo is intentionally unquoted: each word is a make target
    if ! make -j"$JOBS" $combo > /dev/null; then
        echo "Error: build failed for variant: $label"
        exit 1
    fi
done

make clean > /dev/null
echo "All ${#COMBOS[@]} variant combinations built OK"
