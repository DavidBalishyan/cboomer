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
    float cellSize = 0.02;
    vec2 uv = floor(effective_texcoord / cellSize) * cellSize + cellSize * 0.5;
    vec4 texel = texture(tex, uv);
    color = mix(
        texel, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
