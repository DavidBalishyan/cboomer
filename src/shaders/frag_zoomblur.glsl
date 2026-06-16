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

    vec2 cursorUV = cursor.xy / windowSize;
    vec2 dir = uv - cursorUV;
    float dist = length(dir);
    dir = normalize(dir);

    int samples = 24;
    vec4 accum = vec4(0.0);
    float blurAmount = 0.4;
    for (int i = 0; i < samples; i++) {
        float t = float(i) / float(samples - 1);
        vec2 sampleUV = uv - dir * t * dist * blurAmount;
        accum += texture(tex, sampleUV);
    }
    accum /= float(samples);

    color = mix(
        accum, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
