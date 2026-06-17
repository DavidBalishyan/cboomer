#version 120
attribute vec2 position;
attribute vec2 texcoord;
uniform vec2 windowSize;
varying vec2 vTexcoord;

void main() {
    vec2 clip = (position / windowSize) * 2.0 - 1.0;
    gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    vTexcoord = texcoord;
}
