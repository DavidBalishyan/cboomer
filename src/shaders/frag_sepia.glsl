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
    float r = dot(texel.rgb, vec3(0.393, 0.769, 0.189));
    float g = dot(texel.rgb, vec3(0.349, 0.686, 0.168));
    float b = dot(texel.rgb, vec3(0.272, 0.534, 0.131));
    vec4 sepia = vec4(r, g, b, texel.a);
    color = mix(
        sepia, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
