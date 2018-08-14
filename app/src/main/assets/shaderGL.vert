#version 100
precision mediump float;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 normalMatrix;

attribute vec3 inPosition;
attribute vec3 inColor;
attribute vec2 inTexCoord;
attribute vec3 inNormal;

varying vec3 fragColor;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying vec3 fragPosition;

void main() {
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    /* The transpose and inverse functions are not available
       in GLSL 100, so we passed in the normal matrix. */
    fragNormal = normalize(mat3(normalMatrix) * inNormal);

    fragPosition = vec3(model * vec4(inPosition, 1.0));

    gl_Position = proj * view * model * vec4(inPosition, 1.0);
}
