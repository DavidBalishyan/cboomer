#version 120
uniform sampler2D fontTex;
uniform vec3 textColor;
varying vec2 vTexcoord;

void main() {
    float a = texture2D(fontTex, vTexcoord).r;
    gl_FragColor = vec4(textColor, a);
}
