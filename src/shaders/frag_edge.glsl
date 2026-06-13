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

    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));

    float gx = 0.0, gy = 0.0;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 offset = vec2(float(i), float(j)) * texelSize;
            float gray = dot(texture(tex, effective_texcoord + offset).rgb, vec3(0.299, 0.587, 0.114));
            float sx = (j == 0 ? 0.0 : (j < 0 ? -1.0 : 1.0)) * (i == 0 ? 2.0 : 1.0);
            float sy = (i == 0 ? 0.0 : (i < 0 ? -1.0 : 1.0)) * (j == 0 ? 2.0 : 1.0);
            gx += gray * sx;
            gy += gray * sy;
        }
    }

    float edge = sqrt(gx * gx + gy * gy);
    vec4 edge_color = vec4(edge, edge, edge, 1.0);

    color = mix(
        edge_color, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
