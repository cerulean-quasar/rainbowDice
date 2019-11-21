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

/**
 * This vertex shader is used to draw just the lines of a shape and have them be any thickness
 * without being limited by the graphics processor and Vulkan driver (like using the
 * VK_POLYGON_MODE_LINE feature would).  It assumes that the shape being outlined is a square.
 */
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inCorner1;
layout(location = 4) in vec3 inCorner2;
layout(location = 5) in vec3 inCorner3;
layout(location = 6) in vec3 inCorner4;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec3 fragCorner1;
layout(location = 4) out vec3 fragCorner2;
layout(location = 5) out vec3 fragCorner3;
layout(location = 6) out vec3 fragCorner4;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;

    mat3 vecUBO = mat3(transpose(inverse(ubo.model)));
    fragNormal = normalize(vecUBO * inNormal);

    fragCorner1 = vec3(ubo.model * vec4(inCorner1, 1.0));
    fragCorner2 = vec3(ubo.model * vec4(inCorner2, 1.0));
    fragCorner3 = vec3(ubo.model * vec4(inCorner3, 1.0));
    fragCorner4 = vec3(ubo.model * vec4(inCorner4, 1.0));
    fragPosition = vec3(ubo.model * vec4(inPosition, 1.0));
}
