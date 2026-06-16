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

void main()
{
    vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);
    vec2 uv = texcoord;
    if (mirror) {
        uv.x = 1 - uv.x;
    }

    // Ripple distortion
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center);
    float wave = sin(dist * 60.0 - time * 4.0) * 0.015;
    vec2 dir = normalize(uv - center);
    vec2 distorted = uv + dir * wave;

    // Slight barrel distortion
    vec2 barrel = (uv - center) * (1.0 + 0.05 * dist * dist);
    distorted = mix(distorted, center + barrel, 0.3);

    vec4 texel = texture(tex, clamp(distorted, 0.001, 0.999));

    color = mix(
        texel, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
