#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "help" ]; then
    echo "Usage: $0 <output-path>"
    echo "Generate build/shaders.h from src/shaders/*.glsl."
    echo ""
    echo "Called by the Makefile during build. Embeds all vertex and"
    echo "fragment shaders as C string constants."
    exit 0
fi

OUT="${1:?Usage: $0 <output-path>}"

FRAG_NAMES="frag_invert frag_crt frag_grayscale frag_edge frag_vhsglitch frag_distortion frag_zoomblur frag_posterize frag_pixelate frag_sepia frag_emboss"

{
    printf '#ifndef SHADERS_H_\n#define SHADERS_H_\n\n'

    printf 'static const char VERT_SRC[] = "'
    awk '{printf "%s\\n", $0}' src/shaders/vert.glsl
    printf '";\n'

    printf 'static const char FRAG_SRC[] = "'
    awk '{printf "%s\\n", $0}' src/shaders/frag.glsl
    printf '";\n'

    for f in $FRAG_NAMES; do
        n=$(echo "$f" | sed 's/^frag_//' | tr '[:lower:]' '[:upper:]')
        printf "static const char FRAG_%s_SRC[] = \"" "$n"
        awk '{printf "%s\\n", $0}' "src/shaders/$f.glsl"
        printf '";\n'
    done

    printf 'static const char OSD_VERT_SRC[] = "'
    awk '{printf "%s\\n", $0}' src/shaders/osd_vert.glsl
    printf '";\n'

    printf 'static const char OSD_FRAG_SRC[] = "'
    awk '{printf "%s\\n", $0}' src/shaders/osd_frag.glsl
    printf '";\n'
    printf '\n'

    printf '#endif // SHADERS_H_\n'
} > "$OUT"
