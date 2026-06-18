#!/usr/bin/env bash
set -euo pipefail

CMD="${1:-help}"
NAME="${2:-}"

case "$CMD" in
    new)
        if [ -z "$NAME" ]; then
            echo "Usage: $0 new <name>"
            exit 1
        fi
        FNAME="frag_$NAME"
        SHADER="src/shaders/$FNAME.glsl"
        MAKEFILE="Makefile"
        GENSHADERS="scripts/gen_shaders.sh"
        README="README.md"

        if [ -f "$SHADER" ]; then
            echo "Error: $SHADER already exists"
            exit 1
        fi

        cat > "$SHADER" <<'TEMPLATE'
#version 130
out mediump vec4 color;
in mediump vec2 texcoord;
uniform sampler2D tex;
uniform vec2 cursorPos;
uniform vec2 windowSize;
uniform float flShadow;
uniform float flRadius;
uniform float cameraScale;
uniform bool mirror;

void main()
{
    vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);
    vec2 effective_texcoord = texcoord;
    if (mirror) {
        effective_texcoord.x = 1 - effective_texcoord.x;
    }
    vec4 texel = texture(tex, effective_texcoord);
    // TODO: implement shader effect
    color = mix(
        texel, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
TEMPLATE

        echo "Created $SHADER"

        sed -i "s/^\(FRAG_SHADER_NAMES =.*\)$/\1 $FNAME/" "$MAKEFILE"
        echo "Updated $MAKEFILE"

        sed -i "s/^\(FRAG_NAMES=\"[^\"]*\)\"$/\1 $FNAME\"/" "$GENSHADERS"
        echo "Updated $GENSHADERS"

        sed -i "/^\* \*\*Emboss\b/a * **TODO: $NAME** (\`$FNAME.glsl\`): TODO: describe shader" "$README"
        echo "Updated $README"
        ;;
    remove)
        if [ -z "$NAME" ]; then
            echo "Usage: $0 remove <name>"
            exit 1
        fi
        FNAME="frag_$NAME"
        SHADER="src/shaders/$FNAME.glsl"
        MAKEFILE="Makefile"
        GENSHADERS="scripts/gen_shaders.sh"
        README="README.md"

        if [ ! -f "$SHADER" ]; then
            echo "Error: $SHADER does not exist"
            exit 1
        fi

        rm "$SHADER"
        echo "Removed $SHADER"

        sed -i "s/ $FNAME//g" "$MAKEFILE"
        echo "Updated $MAKEFILE"

        sed -i "s/ $FNAME//g" "$GENSHADERS"
        echo "Updated $GENSHADERS"

        sed -i "/\*\*TODO: $NAME\*\*/d" "$README"
        echo "Updated $README"
        ;;
    help|--help|-h)
        echo "Usage: $0 <command> [<name>]"
        echo ""
        echo "Commands:"
        echo "  new <name>     Scaffold a new fragment shader"
        echo "  remove <name>  Remove a fragment shader"
        echo "  ls|list        List all fragment shaders"
        echo ""
        echo "'new' creates src/shaders/frag_<name>.glsl from a template,"
        echo "registers it in the Makefile and gen_shaders.sh, and adds a"
        echo "placeholder in the README shader table."
        echo ""
        echo "'remove' deletes the file and reverts those changes."
        ;;
    ls|list)
        for f in src/shaders/frag_*.glsl; do
            [ -f "$f" ] || continue
            basename="${f#src/shaders/frag_}"
            basename="${basename%.glsl}"
            echo "$basename"
        done
        ;;
    *)
        echo "Unknown command: $CMD"
        echo "Run '$0 help' for usage."
        exit 1
        ;;
esac
