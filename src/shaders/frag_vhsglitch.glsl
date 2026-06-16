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
uniform float time;

float hash(vec2 c) {
    return fract(sin(dot(c, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);
    vec2 uv = texcoord;
    if (mirror) {
        uv.x = 1 - uv.x;
    }

    float row = floor(uv.y * windowSize.y);
    float section = floor(uv.y * windowSize.y * 0.05);

    // Horizontal block shift
    float shift = hash(vec2(section, floor(time * 6.0))) * 0.04;
    vec2 shifted = uv + vec2(shift, 0.0);

    // Color channel offset
    float co = (hash(vec2(section, floor(time * 4.0) + 1.0)) - 0.5) * 0.01;
    vec2 rUv = shifted + vec2(co, 0.0);
    vec2 bUv = shifted - vec2(co, 0.0);

    float r = texture(tex, rUv).r;
    float g = texture(tex, shifted).g;
    float b = texture(tex, bUv).b;
    vec4 texel = vec4(r, g, b, 1.0);

    // Jitter bar - horizontal strip of noise
    float jitter = hash(vec2(section, floor(time * 10.0) + 2.0));
    if (jitter > 0.98) {
        float n = hash(vec2(gl_FragCoord.xy + floor(time * 60.0)));
        texel = mix(texel, vec4(n, n, n, 1.0), 0.7);
    }

    // Dropout line
    float drop = hash(vec2(section, floor(time * 8.0) + 3.0));
    if (drop > 0.995) {
        texel = vec4(1.0, 1.0, 1.0, 1.0);
    }

    color = mix(
        texel, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
