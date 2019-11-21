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
varying vec3 fragNormal;
varying vec3 fragPosition;
varying vec3 fragCorner1;
varying vec3 fragCorner2;
varying vec3 fragCorner3;
varying vec3 fragCorner4;

uniform vec3 viewPosition;

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
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 lightpos = vec3(0.0, -1.0, 1.0);
    vec3 lightDirection = normalize(lightpos - fragPosition);
    vec3 viewDirection = normalize(viewPosition - fragPosition);
    vec3 halfWayDirection = normalize(lightDirection + viewDirection);

    bool nearEdges = false;
    // consider the distance from the edges of the shape. fragCorner1-fragCorner4 are the
    // vectors to the corners of the shape making up a face of the die.
    vec3 f1 = (fragCorner1 + fragCorner2)/2.0;
    vec3 sideNormal1 = normalize(cross(fragCorner1 - fragCorner2, fragNormal));
    vec3 f2 = (fragCorner2 + fragCorner3)/2.0;
    vec3 sideNormal2 = normalize(cross(fragCorner2 - fragCorner3, fragNormal));
    vec3 f3 = (fragCorner3 + fragCorner4)/2.0;
    vec3 sideNormal3 = normalize(cross(fragCorner3 - fragCorner4, fragNormal));
    vec3 f4 = (fragCorner4 + fragCorner1)/2.0;
    vec3 sideNormal4 = normalize(cross(fragCorner4 - fragCorner1, fragNormal));

    float factor = 0.05;
    if (proximity(fragPosition, sideNormal1, factor, f1) <= 0.0) {
        nearEdges = true;
    } else if (proximity(fragPosition, sideNormal2, factor, f2) <= 0.0) {
        nearEdges = true;
    } else if (proximity(fragPosition, sideNormal3, factor, f3) <= 0.0) {
        nearEdges = true;
    } else if (proximity(fragPosition, sideNormal4, factor, f4) <= 0.0) {
        nearEdges = true;
    }

    vec3 color = vec3(fragColor.r, fragColor.g, fragColor.b);
    float alpha = fragColor.a;

    /**
     * reverse the direction of the normal because we want the inside of the box to be lit,
     * not the outside.  But the fragNormal points out of the box to make the proximity to
     * edges function work.
     */
    float diff = max(dot(-fragNormal, lightDirection), 0.0);
    vec3 diffuse = diff * color;

    float ambientFactor = 0.3;
    vec3 ambient = ambientFactor * color;
    if (!nearEdges) {
        alpha = 0.25;
    }
    gl_FragColor = vec4(ambient + diffuse, alpha);
}
