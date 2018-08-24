#version 100
precision mediump float;

varying vec3 fragColor;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying vec3 fragPosition;

uniform sampler2D texSampler;
uniform vec3 viewPosition;

void main() {
    vec3 color;
    if (texture2D(texSampler, vec2(1.0-fragTexCoord.x, fragTexCoord.y)).a != 0.0) {
        color = vec3(1.0 - fragColor.r, 1.0 - fragColor.g, 1.0 - fragColor.b);
    } else {
        color = fragColor;
    }

    float shininess = 16.0;
    vec3 lightpos = vec3(0.0, 5.0, 5.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 lightDirection = normalize(lightpos - fragPosition);
    vec3 viewDirection = normalize(viewPosition - fragPosition);
    vec3 halfWayDirection = normalize(lightDirection + viewDirection);
    float spec = pow(max(dot(fragNormal, halfWayDirection), 0.0), shininess);
    vec3 specular = lightColor * spec;
    float diff = max(dot(fragNormal, lightDirection), 0.0);
    vec3 diffuse = diff * color;

    vec3 ambient = 0.5 * color;
    gl_FragColor = vec4(ambient + diffuse + specular, 1.0);
}
