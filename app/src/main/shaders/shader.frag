/**
 * Copyright 2018 Cerulean Quasar. All Rights Reserved.
 *
 *  This file is part of RainbowDice.
 *
 *  RainbowDice is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RainbowDice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RainbowDice.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;

layout(binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform UniformBufferObject {
    vec3 pos;
} viewPoint;


layout(location = 0) out vec4 outColor;

void main() {
    vec3 color;
    float alpha;
    if (texture(texSampler, fragTexCoord).r != 0) {
        color = vec3(1.0 - fragColor.r, 1.0 - fragColor.g, 1.0 - fragColor.b);
        alpha = 1.0;
    } else {
        color = fragColor;
        alpha = 0.7;
    }

    float shininess = 16.0;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 lightpos = vec3(0.0, -5.0, 5.0);
    vec3 lightDirection = normalize(lightpos - fragPosition);
    vec3 viewDirection = normalize(viewPoint.pos - fragPosition);
    vec3 halfWayDirection = normalize(lightDirection + viewDirection);
    float spec = pow(max(dot(fragNormal, halfWayDirection), 0.0), shininess);
    vec3 specular = lightColor * spec;

    float diff = max(dot(fragNormal, lightDirection), 0.0);
    vec3 diffuse = diff * color;

    vec3 ambient = 0.5 * color;
    outColor = vec4(ambient + diffuse + specular, alpha);
}
