#version 100
precision mediump float;
uniform mat4 MVP;

attribute vec3 inPosition;
attribute vec3 inColor;
attribute vec2 inTexCoord;

varying vec3 fragColor;
varying vec2 fragTexCoord;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
