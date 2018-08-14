#version 100
precision mediump float;

varying vec3 fragColor;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying vec3 fragPosition;

uniform sampler2D texSampler;

void main() {
    vec3 color;
    if (texture2D(texSampler, vec2(1.0-fragTexCoord.x, fragTexCoord.y)).a != 0.0) {
        color = vec3(1.0 - fragColor.r, 1.0 - fragColor.g, 1.0 - fragColor.b);
    } else {
        color = fragColor;
    }

    vec3 lightpos = vec3(0.0, 0.0, 5.0);
    vec3 lightDirection = normalize(lightpos - fragPosition);
    float diff = max(dot(fragNormal, lightDirection), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

    vec3 ambient = vec3(0.3, 0.3, 0.3);
    gl_FragColor = vec4((ambient + diffuse) * color, 1.0);
}
