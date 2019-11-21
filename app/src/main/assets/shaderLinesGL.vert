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
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 normalMatrix;

attribute vec3 inPosition;
attribute vec4 inColor;
attribute vec3 inNormal;
attribute vec3 inCorner1;
attribute vec3 inCorner2;
attribute vec3 inCorner3;
attribute vec3 inCorner4;

varying vec4 fragColor;
varying vec3 fragNormal;
varying vec3 fragPosition;
varying vec3 fragCorner1;
varying vec3 fragCorner2;
varying vec3 fragCorner3;
varying vec3 fragCorner4;

void main() {
    fragColor = inColor;

    /* The transpose and inverse functions are not available
       in GLSL 100, so we passed in the normal matrix. */
    fragNormal = normalize(mat3(normalMatrix) * inNormal);

    fragCorner1 = vec3(model * vec4(inCorner1, 1.0));
    fragCorner2 = vec3(model * vec4(inCorner2, 1.0));
    fragCorner3 = vec3(model * vec4(inCorner3, 1.0));
    fragCorner4 = vec3(model * vec4(inCorner4, 1.0));
    fragPosition = vec3(model * vec4(inPosition, 1.0));

    gl_Position = proj * view * model * vec4(inPosition, 1.0);
}
