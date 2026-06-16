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

    vec4 topLeft = texture(tex, effective_texcoord + vec2(-1.0, -1.0) * texelSize);
    vec4 botRight = texture(tex, effective_texcoord + vec2(1.0, 1.0) * texelSize);

    float grayTopLeft = dot(topLeft.rgb, vec3(0.299, 0.587, 0.114));
    float grayBotRight = dot(botRight.rgb, vec3(0.299, 0.587, 0.114));

    float diff = grayBotRight - grayTopLeft + 0.5;
    vec4 embossed = vec4(diff, diff, diff, 1.0);

    color = mix(
        embossed, vec4(0.0, 0.0, 0.0, 0.0),
        length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);
}
