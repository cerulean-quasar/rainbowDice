#version 100
precision mediump float;

varying vec3 fragColor;
varying vec2 fragTexCoord;

uniform sampler2D texSampler;

void main() {
    if (texture2D(texSampler, vec2(1.0-fragTexCoord.x, fragTexCoord.y)).a != 0.0) {
        gl_FragColor = vec4(1.0 - fragColor.r, 1.0 - fragColor.g, 1.0 - fragColor.b, 1.0);
    } else {
        gl_FragColor = vec4(fragColor, 1.0);
    }
}
