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
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "text.hpp"
#include "dice.hpp"

const float DicePhysicsModel::errorVal = 0.15f;
const float DicePhysicsModel::viscosity = 0.002f;
const float DicePhysicsModel::maxposx = 0.5f;
const float DicePhysicsModel::maxposy = 0.5f;
const float DicePhysicsModel::maxposz = 1.0f;
const float DicePhysicsModel::radius = 0.2f;
const std::vector<glm::vec3> DicePhysicsModel::colors = {
        {1.0f, 0.0f, 0.0f}, // red
        {1.0f, 0.5f, 0.0f}, // orange
        {1.0f, 1.0f, 0.0f}, // yellow
        {0.0f, 1.0f, 0.0f}, // green
        {0.0f, 0.0f, 1.0f}, // blue
        {1.0f, 0.0f, 1.0f}  // purple
};

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);

    /* move to the next data entry after each vertex.  VK_VERTEX_INPUT_RATE_INSTANCE
     * moves to the next data entry after each instance, but we are not using instanced
     * rendering
     */
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

    /* position */
    attributeDescriptions[0].binding = 0; /* binding description to use */
    attributeDescriptions[0].location = 0; /* matches the location in the vertex shader */
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    /* color */
    attributeDescriptions[1].binding = 0; /* binding description to use */
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    /* texture coordinate */
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    /* describes which texture to use */
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[3].offset = offsetof(Vertex, textureToUse);

    return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord && textureToUse == other.textureToUse;
}

void DicePhysicsModel::setView() {
    /* glm::lookAt takes the eye position, the center position, and the up axis as parameters */
    ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

}

void DicePhysicsModel::updatePerspectiveMatrix(int surfaceWidth, int surfaceHeight) {
    /* perspective matrix: takes the perspective projection, the aspect ratio, near and far
     * view planes.
     */
    ubo.proj = glm::perspective(glm::radians(45.0f), surfaceWidth / (float) surfaceHeight, 0.1f, 10.0f);

    /* GLM has the y axis inverted from Vulkan's perspective, invert the y-axis on the
     * projection matrix.
     */
    ubo.proj[1][1] *= -1;
}

void DicePhysicsModel::resetPosition() {
    prevTime = std::chrono::high_resolution_clock::now();
    stopped = false;
    position = glm::vec3();
    velocity = glm::vec3();
    qTotalRotated = glm::quat();
    acceleration = glm::vec3();
    angularSpeed = 0;
    spinAxis = glm::vec3();
}

void DicePhysicsModel::updateAcceleration(float x, float y, float z) {
    acceleration.x = x;
    acceleration.y = y;
    acceleration.z = z;
}

void DicePhysicsModel::calculateBounce(DicePhysicsModel *other) {
    if (glm::length(position - other->position) < radius) {
        // they are almost at the exact same spot, just choose a direction to bounce...
        // Using radius instead of 2*radius because we don't want to hit this condition
        // often since it makes the dice animation look jagged.  Give the else if condition a
        // chance to fix the problem instead.
        position.x += radius;
        other->position.x -= radius;
        velocity.y += errorVal;
        other->velocity.y -= errorVal;
    } else if (glm::length(position - other->position) < 2 * radius) {
        // the dice are too close, they need to bounce off of each other
        glm::vec3 norm = glm::normalize(position - other->position);
        float dot = glm::dot(norm, velocity);
        if (fabs(dot) < errorVal) {
            // The speed of approach is near 0.  make it some larger value so that
            // the dice move apart from each other.
            if (dot < 0) {
                dot = -errorVal;
            } else {
                dot = errorVal;
            }
        }
        velocity = velocity - norm * dot * 2.0f;
        dot = glm::dot(norm, other->velocity);
        if (fabs(dot) < errorVal) {
            // The speed of approach is near 0.  make it some larger value so that
            // the dice move apart from each other.
            if (dot < 0) {
                dot = -errorVal;
            } else {
                dot = errorVal;
            }
        }
        other->velocity = other->velocity - norm * dot * 2.0f;
    }
}

void DicePhysicsModel::updateModelMatrix() {
    // reset the position in the case that it got stuck outside the boundry
    if (position.x < -maxposx) {
        position.x = -maxposx;
    } else if (position.x > maxposx) {
        position.x = maxposx;
    }
    if (position.y < -maxposy) {
        position.y = -maxposy;
    } else if (position.y > maxposy){
        position.y = maxposy;
    }
    if (position.z < -maxposz) {
        position.z = -maxposz;
    } else if (position.z > maxposz){
        position.z = maxposz;
    }

    if (stopped) {
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, radius));
        glm::mat4 rotate = glm::toMat4(qTotalRotated);
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        ubo.model = translate * rotate * scale;
        return;
    }

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
    prevTime = currentTime;
    float speed = glm::length(velocity);

    if (speed != 0) {
        position += velocity*time;
        if (position.x > maxposx || position.x < -maxposx) {
            /* the die will start to spin when it hits the wall */
            glm::vec3 spinAxisAdded = glm::cross(velocity,glm::vec3(1.0f, 0.0f, 0.0f));
            spinAxis = spinAxis + spinAxisAdded;
            if (glm::length(spinAxis) > 0) {
                spinAxis = glm::normalize(spinAxis);
                angularSpeed += 10*glm::length(glm::vec3(0.0f, velocity.y, velocity.z));
            }

            velocity.x *= -1;
        }
        if (position.y > maxposy || position.y < -maxposy) {
            /* the die will start to spin when it hits the wall */
            glm::vec3 spinAxisAdded = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), velocity);
            spinAxis = spinAxis + spinAxisAdded;
            if (glm::length(spinAxis) > 0) {
                spinAxis = glm::normalize(spinAxis);
                angularSpeed += 10*glm::length(glm::vec3(velocity.x, 0.0f, velocity.z));
            }

            velocity.y *= -1;
        }
        if (position.z > maxposz || position.z < -maxposz) {
            /* the die will start to spin when it hits the wall */
            glm::vec3 spinAxisAdded = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), velocity);
            spinAxis = spinAxis + spinAxisAdded;
            if (glm::length(spinAxis) > 0) {
                spinAxis = glm::normalize(spinAxis);
                angularSpeed += 10*glm::length(glm::vec3(velocity.x, velocity.y, 0.0f));
            }

            velocity.z *= -1;
        }
        velocity -= glm::normalize(velocity) * viscosity * speed * speed;
    }

    velocity = velocity + acceleration * time;

    if ((position.z > maxposz || maxposz - position.z < errorVal) && fabs(velocity.z) < errorVal) {
        stopped = true;

        std::string upFaceSymbol = calculateUpFace();
        result = upFaceSymbol;

        angularSpeed = 0;
        velocity.x = 0;
        velocity.y = 0;
        velocity.z = 0;
        position.z = maxposz;
    }

    if (angularSpeed != 0 && glm::length(spinAxis) > 0) {
        glm::quat q = glm::angleAxis(angularSpeed*time, spinAxis);
        qTotalRotated = glm::normalize(q * qTotalRotated);
        angularSpeed -= viscosity * angularSpeed * angularSpeed;
    }

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, radius));
    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
    ubo.model = translate * rotate * scale;
}

void DiceModelCube::loadModel(TextureAtlas &texAtlas) {
    Vertex vertex = {};

    // vertices
    for (uint32_t i = 0; i < 4; i ++) {
        // top
        switch (i) {
        case 0:
            vertex.texCoord = {0, 1};
            break;
        case 1:
            vertex.texCoord = {0, 0};
            break;
        case 2:
            vertex.texCoord = {1, 0};
            break;
        case 3:
            vertex.texCoord = {1, 1};
            break;
        }
        vertex.textureToUse = texAtlas.getArrayIndex(symbols[0]);
        vertex.color = colors[i%colors.size()];
        cubeTop(vertex, i);
        vertices.push_back(vertex);

        //bottom
        switch (i) {
        case 0:
            vertex.texCoord = {0, 0};
            break;
        case 1:
            vertex.texCoord = {0, 1};
            break;
        case 2:
            vertex.texCoord = {1, 1};
            break;
        case 3:
            vertex.texCoord = {1, 0};
            break;
        }
        vertex.textureToUse = texAtlas.getArrayIndex(symbols[1]);
        vertex.color = colors[(i+3) %colors.size()];
        cubeBottom(vertex, i);
        vertices.push_back(vertex);
    }

    // indices
    // top
    indices.push_back(0);
    indices.push_back(6);
    indices.push_back(2);

    indices.push_back(2);
    indices.push_back(6);
    indices.push_back(4);

    // bottom
    indices.push_back(7);
    indices.push_back(1);
    indices.push_back(3);

    indices.push_back(7);
    indices.push_back(3);
    indices.push_back(5);

    // sides
    for (uint32_t i = 0; i < 4; i ++) {
        vertex.color = colors[i%colors.size()];
        vertex.textureToUse = texAtlas.getArrayIndex(symbols[(i+2)%symbols.size()]);

        cubeTop(vertex, i);
        vertex.texCoord = {0,0};
        vertices.push_back(vertex);
        printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);

        cubeTop(vertex, (i+1)%4);
        vertex.texCoord = {0,1};
        vertices.push_back(vertex);
        printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);

        cubeBottom(vertex, i);
        vertex.texCoord = {1,0};
        vertices.push_back(vertex);
        printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);

        cubeBottom(vertex, (i+1)%4);
        vertex.texCoord = {1,1};
        vertices.push_back(vertex);
        printf("%f %f %f\n\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);

        indices.push_back(9+4*i);
        indices.push_back(11+4*i);
        indices.push_back(8+4*i);

        indices.push_back(8+4*i);
        indices.push_back(11+4*i);
        indices.push_back(10+4*i);

    }
}

void DiceModelCube::cubeTop(Vertex &vertex, uint32_t i) {
    const float pi = glm::acos(-1.0f);
    vertex.pos = {glm::cos(2*i*pi/4), 1.0f, glm::sin(2*i*pi/4)};
}

void DiceModelCube::cubeBottom(Vertex &vertex, uint32_t i) {
    const float pi = glm::acos(-1.0f);
    vertex.pos = {glm::cos(2*i*pi/4), -0.5f, glm::sin(2*i*pi/4)};
}

std::string DiceModelCube::calculateUpFace() {
    const float pi = glm::acos(-1.0f);
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);
    glm::vec3 upPerpendicular;

    float angleMin = pi;
    float angle;
    uint32_t upFace = 0;

    /* what was the top face at the start (y being up) */
    glm::vec4 p04 = ubo.model * glm::vec4(vertices[2].pos, 1.0f);
    glm::vec4 q4 = ubo.model * glm::vec4(vertices[0].pos, 1.0f);
    glm::vec4 r4 = ubo.model * glm::vec4(vertices[6].pos, 1.0f);
    glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
    glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
    glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
    glm::vec3 perpendicularFaceVec = glm::normalize(glm::cross(q-r, q-p0));
    angle = glm::acos(glm::dot(perpendicularFaceVec, zaxis));
    if (angleMin > angle) {
        upPerpendicular = perpendicularFaceVec;
        angleMin = angle;
        upFace = 0;
    }

    /* what was the bottom face at the start */
    p04 = ubo.model * glm::vec4(vertices[7].pos, 1.0f);
    q4 = ubo.model * glm::vec4(vertices[1].pos, 1.0f);
    r4 = ubo.model * glm::vec4(vertices[3].pos, 1.0f);
    p0 = glm::vec3(p04.x, p04.y, p04.z);
    q = glm::vec3(q4.x, q4.y, q4.z);
    r = glm::vec3(r4.x, r4.y, r4.z);
    perpendicularFaceVec = glm::normalize(glm::cross(q-r, q-p0));
    angle = glm::acos(glm::dot(perpendicularFaceVec, zaxis));
    if (angleMin > angle) {
        upPerpendicular = perpendicularFaceVec;
        angleMin = angle;
        upFace = 1;
    }

    /* what were the side faces at the start */
    for (uint32_t i = 0; i < 4; i++) {
        p04 = ubo.model * glm::vec4(vertices[8+4*i].pos, 1.0f);
        q4 = ubo.model * glm::vec4(vertices[9+4*i].pos, 1.0f);
        r4 = ubo.model * glm::vec4(vertices[11+4*i].pos, 1.0f);
        p0 = glm::vec3(p04.x, p04.y, p04.z);
        q = glm::vec3(q4.x, q4.y, q4.z);
        r = glm::vec3(r4.x, r4.y, r4.z);
        perpendicularFaceVec = glm::normalize(glm::cross(q-r, q-p0));
        angle = glm::acos(glm::dot(perpendicularFaceVec, zaxis));
        if (angleMin > angle) {
            upPerpendicular = perpendicularFaceVec;
            angleMin = angle;
            upFace = i+2;
        }
    }

    glm::quat quaternian = glm::angleAxis(angleMin, glm::normalize(glm::cross(upPerpendicular, zaxis)));
    qTotalRotated = glm::normalize(quaternian * qTotalRotated);

    return symbols[upFace%symbols.size()];
}

void DiceModelHedron::loadModel(TextureAtlas &texAtlas) {
    sides = symbols.size();
    while (sides < 6 || sides % 2 != 0) {
        sides *= 2;
    }

    const float pi = glm::acos(-1.0f);

    // top
    for (uint32_t i = 0; i < sides/2; i ++) {
        // bottom
        glm::vec3 p0 = {0.0f, -1.0f, 0.0f};
        glm::vec3 q = {glm::cos(4*i*pi/sides), 0.0f, glm::sin(4*i*pi/sides)};
        glm::vec3 r = {glm::cos(4.0f*((i+1)%(sides/2))*pi/sides), 0.0f, glm::sin(4*((i+1)%(sides/2))*pi/sides)};
        addVertices(p0, q, r, i, texAtlas);

        // top
        p0 = {0.0f, 1.0f, 0.0f};
        q = {glm::cos(4.0f*((i+1)%(sides/2))*pi/sides), 0.0f, glm::sin(4*((i+1)%(sides/2))*pi/sides)};
        r = {glm::cos(4*i*pi/sides), 0.0f, glm::sin(4*i*pi/sides)};
        addVertices(p0, q, r, i+sides/2, texAtlas);
    }

    // indices - not really using these
    for (uint32_t i = 0; i < sides*15; i ++) {
        indices.push_back(i);
    }
}

void DiceModelHedron::addVertices(glm::vec3 p0, glm::vec3 q, glm::vec3 r, uint32_t i, TextureAtlas &texAtlas) {
    TextureImage tex = texAtlas.getImage(symbols[i%symbols.size()]);
    int textureToUse = texAtlas.getArrayIndex(symbols[i%symbols.size()]);
    float a = tex.width/(float)tex.height;

    Vertex vertex = {};

    float k = a * glm::length((r+q)/2.0f-p0) / glm::length(r-q);

    glm::vec3 p2 = (1.0f - 1.0f/(2.0f * k + 2.0f)) * r + (1.0f/(2.0f*k+2.0f)) * q;
    glm::vec3 p3 = 1.0f/(2.0f*k+2.0f) * r + (1.0f-1.0f/(2.0f*k+2.0f)) * q;
    glm::vec3 p1 = p2 - 1.0f/(1.0f+k) * ((r+q)/2.0f - p0);
    glm::vec3 p1prime = p3 - 1.0f/(1.0f+k) * ((r+q)/2.0f - p0);

    // Top triangle
    // not really using textures for this triangle
    vertex.pos = p0;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[i%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p1prime;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p1;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    // bottom left triangle
    // not really using textures for this triangle
    vertex.pos = q;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+2) %colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p3;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+2) %colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p1prime;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    // bottom right triangle
    // not really using textures for this triangle
    vertex.pos = r;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+2)%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p1;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+2)%colors.size()];
    vertex.textureToUse = 0;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    // bottom text triangle
    vertex.pos = p1prime;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = textureToUse;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p3;
    vertex.texCoord = {0.0f, 1.0f};
    vertex.color = colors[(i+2)%colors.size()];
    vertex.textureToUse = textureToUse;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.texCoord = {1.0f, 1.0f};
    vertex.color = colors[(i+2)%colors.size()];
    vertex.textureToUse = textureToUse;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    // top text triangle
    vertex.pos = p1prime;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = textureToUse;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.texCoord = {1.0f, 1.0f};
    vertex.color = colors[(i+2)%colors.size()];
    vertex.textureToUse = textureToUse;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);

    vertex.pos = p1;
    vertex.texCoord = {1.0f, 0.0f};
    vertex.color = colors[(i+1)%colors.size()];
    vertex.textureToUse = textureToUse;
    printf("%f %f %f\n", vertex.pos.x, vertex.pos.y, vertex.pos.z);
    vertices.push_back(vertex);
}

std::string DiceModelHedron::calculateUpFace() {
    const float pi = glm::acos(-1.0f);
    const uint32_t nbrVerticesPerFace = vertices.size()/sides;
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);
    glm::vec3 upPerpendicular;

    float angleMin = pi;
    float angle;
    uint32_t upFace = 0;
    for (uint32_t i = 0; i < sides; i++) {
        glm::vec4 p04 = ubo.model * glm::vec4(vertices[i * nbrVerticesPerFace].pos, 1.0f);
        glm::vec4 q4 = ubo.model * glm::vec4(vertices[3 + i * nbrVerticesPerFace].pos, 1.0f);
        glm::vec4 r4 = ubo.model * glm::vec4(vertices[6 + i * nbrVerticesPerFace].pos, 1.0f);
        glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
        glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
        glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
        glm::vec3 perpendicularFaceVec = glm::normalize(glm::cross(q-r, q-p0));
        angle = glm::acos(glm::dot(perpendicularFaceVec, zaxis));
        if (angleMin > angle) {
            upPerpendicular = perpendicularFaceVec;
            if (i%2 == 0) {
                upFace = i/2;
            } else {
                upFace = i/2 + sides/2;
            }
            angleMin = angle;
        }
    }

    glm::quat quaternian = glm::angleAxis(angleMin, glm::normalize(glm::cross(upPerpendicular, zaxis)));
    qTotalRotated = glm::normalize(quaternian * qTotalRotated);

    return symbols[upFace%symbols.size()];
}

