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

    vec2 center = vec2(0.5, 0.5);
    float dist = distance(texcoord, center);
    vec2 dir = texcoord - center;
    float strength = dist * 0.005;

    vec2 effective_texcoord = texcoord;
    if (mirror) {
        effective_texcoord.x = 1 - effective_texcoord.x;
    }

    float r = texture(tex, effective_texcoord - dir * strength).r;
    float g = texture(tex, effective_texcoord).g;
    float b = texture(tex, effective_texcoord + dir * strength).b;
    vec4 texel = vec4(r, g, b, 1.0);

    float scanline = sin(texcoord.y * windowSize.y * 3.14159) * 0.05;
    texel.rgb -= scanline;

    float vignette = 1.0 - dist * dist * 0.5;
    texel.rgb *= vignette;

    color = mix(
        texel, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
