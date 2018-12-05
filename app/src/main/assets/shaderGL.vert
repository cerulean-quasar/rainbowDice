#version 100
precision highp float;
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
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 normalMatrix;

attribute vec3 inPosition;
attribute vec3 inColor;
attribute vec2 inTexCoord;
attribute vec3 inNormal;
attribute vec3 inCornerNormal;
attribute vec3 inCorner1;
attribute vec3 inCorner2;
attribute vec3 inCorner3;
attribute vec3 inCorner4;
attribute vec3 inCorner5;
attribute float inMode;

varying vec3 fragColor;
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

void main() {
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    fragMode = inMode;

    /* The transpose and inverse functions are not available
       in GLSL 100, so we passed in the normal matrix. */
    fragNormal = normalize(mat3(normalMatrix) * inNormal);
    fragCornerNormal = normalize(mat3(normalMatrix) * inCornerNormal);

    if (length(inCorner1) < 1000.0) {
        fragCorner1 = vec3(model * vec4(inCorner1, 1.0));
    } else {
        fragCorner1 = inCorner1;
    }

    if (length(inCorner2) < 1000.0) {
        fragCorner2 = vec3(model * vec4(inCorner2, 1.0));
    } else {
        fragCorner2 = inCorner2;
    }

    if (length(inCorner3) < 1000.0) {
        fragCorner3 = vec3(model * vec4(inCorner3, 1.0));
    } else {
        fragCorner3 = inCorner3;
    }

    if (length(inCorner4) < 1000.0) {
        fragCorner4 = vec3(model * vec4(inCorner4, 1.0));
    } else {
        fragCorner4 = inCorner4;
    }

    if (length(inCorner5) < 1000.0) {
        fragCorner5 = vec3(model * vec4(inCorner5, 1.0));
    } else {
        fragCorner5 = inCorner5;
    }

    fragPosition = vec3(model * vec4(inPosition, 1.0));

    gl_Position = proj * view * model * vec4(inPosition, 1.0);
}
