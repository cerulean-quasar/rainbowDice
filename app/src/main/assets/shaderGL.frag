#version 100
precision highp float;
/**
 * Copyright 2019 Cerulean Quasar. All Rights Reserved.
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

varying vec4 fragColor;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying vec3 fragPosition;
varying vec3 fragCornerNormal;
varying vec3 fragCorner1;
varying vec3 fragCorner2;
varying vec3 fragCorner3;
varying vec3 fragCorner4;
varying vec3 fragCorner5;
varying float fragMode;

uniform sampler2D texSampler;
uniform vec3 viewPosition;
uniform int isSelected;
uniform float edgeWidth;

float proximity(vec3 position, vec3 sideNormal, float factor, vec3 sideAvg) {
    if (length(sideNormal) == 0.0) {
        return 1.0;
    }
    float dotprod = dot((position - sideAvg), sideNormal);
    if (dotprod < 0.0) {
        return 1.0;
    }
    return dotprod-factor;
}

void main() {
    vec3 color;
    float alpha = 1.0;
    vec4 textureColor = texture2D(texSampler, vec2(fragTexCoord.x, fragTexCoord.y));
    if (textureColor.a != 0.0) {
        if (textureColor.r == 0.0 && textureColor.g == 0.0 && textureColor.b == 0.0) {
            color = vec3(1.0 - fragColor.r, 1.0 - fragColor.g, 1.0 - fragColor.b);
        } else {
            color = vec3(textureColor.r, textureColor.g, textureColor.b);
        }
        alpha = fragColor.a;
    } else {
        color = vec3(fragColor.r, fragColor.g, fragColor.b);
        alpha = fragColor.a;
    }

    float shininess = 8.0;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 lightpos = vec3(0.0, -5.0, 5.0);
    vec3 lightDirection = normalize(lightpos - fragPosition);
    vec3 viewDirection = normalize(viewPosition - fragPosition);
    vec3 halfWayDirection = normalize(lightDirection + viewDirection);

    vec3 specNormal;
    float factor = edgeWidth;
    float factor2 = 0.005;

    vec3 sideNormal1;
    vec3 sideNormal2;
    vec3 sideNormal3;
    vec3 sideNormal4 = vec3(0.0, 0.0, 0.0);
    vec3 sideNormal5 = vec3(0.0, 0.0, 0.0);

    vec3 f1;
    vec3 f2;
    vec3 f3;
    vec3 f4 = vec3(0.0, 0.0, 0.0);
    vec3 f5 = vec3(0.0, 0.0, 0.0);

    vec3 value = vec3(0.0,0.0,0.0);

    bool nearEdges = false;
    if (fragMode <= 0.0) {
        // consider the distance from the edges of the shape. fragCorner1-fragCorner4 are the
        // vectors to the corners of the shape making up a face of the die.
        f1 = (fragCorner1 + fragCorner2)/2.0;
        f2 = (fragCorner2 + fragCorner3)/2.0;
        sideNormal1 = normalize(cross(fragCorner1 - fragCorner2, fragNormal));
        sideNormal2 = normalize(cross(fragCorner2 - fragCorner3, fragNormal));
        if (length(fragCorner4) < 1000.0) {
            f3 = (fragCorner3 + fragCorner4)/2.0;
            sideNormal3 = normalize(cross(fragCorner3 - fragCorner4, fragNormal));
            if (length(fragCorner5) < 1000.0) {
                f4 = (fragCorner4 + fragCorner5)/2.0;
                f5 = (fragCorner5 + fragCorner1)/2.0;
                sideNormal4 = normalize(cross(fragCorner4 - fragCorner5, fragNormal));
                sideNormal5 = normalize(cross(fragCorner5 - fragCorner1, fragNormal));
            } else {
                f4 = (fragCorner4 + fragCorner1)/2.0;
                sideNormal4 = normalize(cross(fragCorner4 - fragCorner1, fragNormal));
            }
        } else {
            f3 = (fragCorner3 + fragCorner1)/2.0;
            sideNormal3 = normalize(cross(fragCorner3 - fragCorner1, fragNormal));
        }

        if (proximity(fragPosition, sideNormal1, factor, f1) <= 0.0) {
            specNormal = normalize(fragCornerNormal);
            nearEdges = true;
        } else if (proximity(fragPosition, sideNormal2, factor, f2) <= 0.0) {
            specNormal = normalize(fragCornerNormal);
            nearEdges = true;
        } else if (proximity(fragPosition, sideNormal3, factor, f3) <= 0.0) {
            specNormal = normalize(fragCornerNormal);
            nearEdges = true;
        } else if (proximity(fragPosition, sideNormal4, factor, f4) <= 0.0) {
            specNormal = normalize(fragCornerNormal);
            nearEdges = true;
        } else if (proximity(fragPosition, sideNormal5, factor, f5) <= 0.0) {
            specNormal = normalize(fragCornerNormal);
            nearEdges = true;
        } else {
            specNormal = fragNormal;
            shininess = 16.0;
        }
    } else {
        // consider distance from center.  fragCorner1 is a vector to the center of the face
        // of the die.  fragCorner2 is a vector to one of the outer points in the triangle.
        if (length(fragCorner2 - fragCorner1) - length(fragPosition - fragCorner1) - factor <= 0.0) {
            specNormal = normalize(fragCornerNormal);
            nearEdges = true;
        } else {
            specNormal = normalize(fragNormal);
            shininess = 16.0;
        }
     }

    if (isSelected > 0 && nearEdges) {
        gl_FragColor = vec4(1.0 - fragColor.r, 1.0 - fragColor.g, 1.0 - fragColor.b, alpha);
    } else {
        float spec = pow(max(dot(specNormal, halfWayDirection), 0.0), shininess);
        vec3 specular = lightColor * spec;
        float diff = max(dot(specNormal, lightDirection), 0.0);
        vec3 diffuse = diff * color;

        float ambientFactor = 0.3;
        if (isSelected > 0) {
            ambientFactor = 1.0;
        }
        vec3 ambient = ambientFactor * color;
        gl_FragColor = vec4(ambient + diffuse + specular, alpha);
    }
}
