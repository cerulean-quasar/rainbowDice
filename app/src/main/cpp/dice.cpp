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
#include "rainbowDiceGlobal.hpp"
#include "random.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits>

float const DicePhysicsModel::errorVal = 0.15f;
float const DicePhysicsModel::viscosity = 2.0f;
float const DicePhysicsModel::radius = 0.2f;
float const DicePhysicsModel::maxposx = 0.5f;
float const DicePhysicsModel::maxposy = 0.5f;
float const DicePhysicsModel::maxposz = 1.0f;

// Time for the stopped animation to complete
float const DicePhysicsModel::stoppedAnimationTime = 0.5f; // seconds

// Time it takes for stopped die to settle to being flat from a cocked position.
float const DicePhysicsModel::goingToStopAnimationTime = 0.2f; // seconds

// Time to wait after the dice settled flat before moving them to the top of the window.
float const DicePhysicsModel::waitAfterDoneTime = 0.6f; // seconds

// the radius of the dice after it has moved to the top of the screen.
float const DicePhysicsModel::stoppedRadius = 0.12f;

std::vector<glm::vec3> const DicePhysicsModel::colors = {
        {1.0f, 0.0f, 0.0f}, // red
        {1.0f, 0.5f, 0.0f}, // orange
        {1.0f, 1.0f, 0.0f}, // yellow
        {0.5f, 1.0f, 0.0f}, // yellow-green
        {0.0f, 1.0f, 0.0f}, // green
        {0.0f, 0.0f, 1.0f}, // blue
        {0.0f, 0.0f, 0.5f}, // indigo
        {1.0f, 0.0f, 1.0f}  // purple
};
unsigned long const Filter::highPassAccelerationMaxSize = 512;
float const DicePhysicsModel::angularSpeedScaleFactor = 5.0f;
float const AngularVelocity::maxAngularSpeed = 10.0f;
Filter DicePhysicsModel::filter;
const float pi = glm::acos(-1.0f);

void checkQuaternion(glm::quat &q) {
    if (glm::length(q) == 0) {
        q = glm::quat();
    }
    if (q.x != q.x || q.y != q.y || q.z != q.z || q.w != q.w) {
        q = glm::quat();
    }
}

bool Vertex::operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
}

void DicePhysicsModel::setView() {
    /* glm::lookAt takes the eye position, the center position, and the up axis as parameters */
    ubo.view = glm::lookAt(viewPoint, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

}

void DicePhysicsModel::updatePerspectiveMatrix(uint32_t surfaceWidth, uint32_t surfaceHeight) {
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
    goingToStop = false;
    animationDone = false;
    animationTime = 0.0f;
    stoppedRotateTime = 0.0f;
    stoppedAngle = 0.0f;
    stoppedRotationAxis = {0.0f, 0.0f, 1.0f};
    doneX = 0.0f;
    doneY = 0.0f;
    stoppedPositionX = 0.0f;
    stoppedPositionY = 0.0f;
    position = glm::vec3();
    velocity = glm::vec3();
    qTotalRotated = glm::quat();
    acceleration = glm::vec3();
    angularVelocity.setAngularSpeed(0);
    angularVelocity.setSpinAxis(glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, radius));
    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
    ubo.model = translate * rotate * scale;

    randomizeUpFace();
    checkQuaternion(qTotalRotated);
}

void DicePhysicsModel::updateAcceleration(float x, float y, float z) {
    glm::vec3 a = {x,y,z};
    acceleration = filter.acceleration(a);
}

void DicePhysicsModel::calculateBounce(DicePhysicsModel *other) {
    if (glm::length(position - other->position) < 2 * radius) {
        // the dice are too close, they need to bounce off of each other

        if (glm::length(position-other->position) == 0) {
            position.x += radius;
            other->position.x -= radius;
        }
        glm::vec3 norm = glm::normalize(position - other->position);
        float length = glm::length(position - other->position);
        if (length < radius) {
            // they are almost at the exact same spot, just choose a direction to bounce...
            // Using radius instead of 2*radius because we don't want to hit this condition
            // often since it makes the dice animation look jagged.  Give the else if condition a
            // chance to fix the problem instead.
            position = position + (radius-length) * norm;
            other->position = other->position - (radius-length) * norm;
        }
        float dot = glm::dot(norm, velocity);
        if (fabs(dot) < errorVal) {
            // The speed of approach is near 0.  make it some larger value so that
            // the dice move apart from each other.
            if (dot < 0) {
                dot = -10*errorVal;
            } else {
                dot = 10*errorVal;
            }
        }
        velocity = velocity - norm * 2.0f * dot;
        dot = glm::dot(norm, other->velocity);
        if (fabs(dot) < errorVal) {
            // The speed of approach is near 0.  make it some larger value so that
            // the dice move apart from each other.
            if (dot < 0) {
                dot = -10*errorVal;
            } else {
                dot = 10*errorVal;
            }
        }
        other->velocity = other->velocity - norm * 2.0f * dot;
    }
}

bool DicePhysicsModel::updateModelMatrix() {
    // reset the position in the case that it got stuck outside the boundry
    if (position.x < -maxposx) {
        position.x = -maxposx;
    } else if (position.x > maxposx) {
        position.x = maxposx;
    }
    if (position.y < -maxposy) {
        position.y = -maxposy;
    } else if (position.y > maxposy) {
        position.y = maxposy;
    }
    if (position.z < -maxposz) {
        position.z = -maxposz;
    } else if (position.z > maxposz) {
        position.z = maxposz;
    }

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
    prevTime = currentTime;

    if (goingToStop) {
        if (animationDone) {
            return false;
        } else if (doneY == 0.0f && stopped) {
            glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
            checkQuaternion(qTotalRotated);
            glm::mat4 rotate = glm::toMat4(qTotalRotated);
            glm::mat4 translate = glm::translate(position);
            ubo.model = translate * rotate * scale;
            return true;
        } else if (stoppedAnimationTime <= animationTime) {
            // all animations after the dice lands are done.
            animationDone = true;
            position.x = doneX;
            position.y = doneY;
            glm::mat4 scale = glm::scale(glm::vec3(stoppedRadius, stoppedRadius, stoppedRadius));
            if (stoppedAngle != 0) {
                glm::quat q = glm::angleAxis(stoppedAngle, stoppedRotationAxis);
                qTotalRotated = glm::normalize(q * qTotalRotated);
            }
            checkQuaternion(qTotalRotated);
            glm::mat4 rotate = glm::toMat4(qTotalRotated);
            glm::mat4 translate = glm::translate(position);
            ubo.model = translate * rotate * scale;
            return true;
        } else if (stopped) {
            // we need to animate the move to the location it is supposed to go when it is done
            // rolling so that it is out of the way of rolling dice.
            animationTime += time;
            if (stoppedAnimationTime < animationTime) {
                animationTime = stoppedAnimationTime;
            }
            float r = radius - (radius - stoppedRadius) / stoppedAnimationTime * animationTime;
            position.x = (doneX - stoppedPositionX) / stoppedAnimationTime * animationTime +
                         stoppedPositionX;
            position.y = (doneY - stoppedPositionY) / stoppedAnimationTime * animationTime +
                         stoppedPositionY;
            glm::mat4 rotate;
            if (stoppedAngle != 0) {
                glm::quat q = glm::angleAxis(stoppedAngle/stoppedAnimationTime*animationTime,
                                             stoppedRotationAxis);
                checkQuaternion(qTotalRotated);
                checkQuaternion(q);
                rotate = glm::toMat4(glm::normalize(q*qTotalRotated));
            } else {
                rotate = glm::toMat4(qTotalRotated);
            }
            glm::mat4 scale = glm::scale(glm::vec3(r, r, r));
            glm::mat4 translate = glm::translate(position);
            ubo.model = translate * rotate * scale;
            return true;
        } else if (waitAfterDoneTime <= stoppedRotateTime) {
            // done settling the dice to the floor.  Wait for some time before moving the die to the
            // top of the screen.
            stopped = true;
            yAlign(upFace);
            glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
            glm::mat4 rotate = glm::toMat4(qTotalRotated);
            glm::mat4 translate = glm::translate(position);
            ubo.model = translate * rotate * scale;
            return true;
        } else if (goingToStopAnimationTime > stoppedRotateTime){
            // we need to animate the dice slowly settling to the floor.
            stoppedRotateTime += time;
            if (goingToStopAnimationTime < stoppedRotateTime) {
                stoppedRotateTime = goingToStopAnimationTime;
            }
            glm::mat4 rotate;
            if (stoppedAngle != 0) {
                glm::quat q = glm::angleAxis(
                        stoppedAngle / goingToStopAnimationTime * stoppedRotateTime,
                        stoppedRotationAxis);
                rotate = glm::toMat4(glm::normalize(q * qTotalRotated));
            } else {
                rotate = glm::mat4(1.0f);
            }
            glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
            glm::mat4 translate = glm::translate(position);
            ubo.model = translate * rotate * scale;
            return true;
        } else {
            stoppedRotateTime += time;
            // stopped animation done, now wait a while and then begin the animation to
            // rotate and move the dice to the top of the screen.
            if (stoppedAngle != 0.0f) {
                glm::quat q = glm::angleAxis(stoppedAngle, stoppedRotationAxis);
                qTotalRotated = glm::normalize(q * qTotalRotated);
                checkQuaternion(qTotalRotated);
                stoppedAngle = 0.0f;
            }
            glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
            glm::mat4 rotate = glm::toMat4(qTotalRotated);
            glm::mat4 translate = glm::translate(position);
            ubo.model = translate * rotate * scale;
            return true;
        }
    }

    float speed = glm::length(velocity);

    if (speed != 0) {
        position += velocity*time;
        if (position.x > maxposx || position.x < -maxposx) {
            /* the die will start to spin when it hits the wall */
            glm::vec3 spinAxisAdded = glm::cross(velocity,glm::vec3(1.0f, 0.0f, 0.0f));
            glm::vec3 spinAxis = angularVelocity.spinAxis() + spinAxisAdded;
            if (glm::length(spinAxis) > 0) {
                spinAxis = glm::normalize(spinAxis);
                angularVelocity.setSpinAxis(spinAxis);
                float angularSpeed = angularVelocity.speed();
                angularSpeed += angularSpeedScaleFactor*glm::length(glm::vec3(0.0f, velocity.y, velocity.z));
                angularVelocity.setAngularSpeed(angularSpeed);
            }

            velocity.x *= -1;
        }
        if (position.y > maxposy || position.y < -maxposy) {
            /* the die will start to spin when it hits the wall */
            glm::vec3 spinAxisAdded = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), velocity);
            glm::vec3 spinAxis = angularVelocity.spinAxis() + spinAxisAdded;
            if (glm::length(spinAxis) > 0) {
                spinAxis = glm::normalize(spinAxis);
                angularVelocity.setSpinAxis(spinAxis);
                float angularSpeed = angularVelocity.speed();
                angularSpeed += angularSpeedScaleFactor*glm::length(glm::vec3(0.0f, velocity.y, velocity.z));
                angularVelocity.setAngularSpeed(angularSpeed);
            }

            velocity.y *= -1;
        }
        if (position.z > maxposz || position.z < -maxposz) {
            /* the die will start to spin when it hits the wall */
            glm::vec3 spinAxisAdded = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), velocity);
            glm::vec3 spinAxis = angularVelocity.spinAxis() + spinAxisAdded;
            if (glm::length(spinAxis) > 0) {
                spinAxis = glm::normalize(spinAxis);
                angularVelocity.setSpinAxis(spinAxis);
                float angularSpeed = angularVelocity.speed();
                angularSpeed += angularSpeedScaleFactor*glm::length(glm::vec3(0.0f, velocity.y, velocity.z));
                angularVelocity.setAngularSpeed(angularSpeed);
            }

            velocity.z *= -1;
        }
        velocity -= velocity * viscosity * time;
    }

    velocity += acceleration * time;

    if ((position.z > maxposz || maxposz - position.z < errorVal) && fabs(velocity.z) < errorVal) {
        goingToStop = true;

        upFace = calculateUpFace();
        std::string upFaceSymbol = symbols[getUpFaceIndex(upFace)%symbols.size()];
        result = upFaceSymbol;

        angularVelocity.setAngularSpeed(0);
        velocity = {0.0f, 0.0f, 0.0f};
        stoppedPositionX = position.x;
        stoppedPositionY = position.y;
        position.z = maxposz;
    }

    if (angularVelocity.speed() != 0 && glm::length(angularVelocity.spinAxis()) > 0) {
        glm::quat q = glm::angleAxis(angularVelocity.speed()*time, angularVelocity.spinAxis());
        qTotalRotated = glm::normalize(q * qTotalRotated);
        float angularSpeed = angularVelocity.speed();
        angularSpeed -= viscosity * angularSpeed * time;
        angularVelocity.setAngularSpeed(angularSpeed);
    }

    glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
    checkQuaternion(qTotalRotated);
    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    glm::mat4 translate = glm::translate(position);
    ubo.model = translate * rotate * scale;

    if (goingToStop) {
        return true;
    }

    float difference = glm::length(position - prevPosition);
    if (difference < 0.01) {
        return false;
    } else {
        prevPosition = position;
        return true;
    }
}

uint32_t DicePhysicsModel::calculateUpFace() {
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);
    glm::vec3 upPerpendicular;
    glm::vec3 perpendicularFaceVec;

    float angleMin = pi;
    float angle;
    uint32_t upFace = 0;

    for (uint32_t i = 0; i < numberFaces; i++) {
        getAngleAxis(i, angle, perpendicularFaceVec);
        if (angleMin > angle) {
            angleMin = angle;
            upPerpendicular = perpendicularFaceVec;
            upFace = i;
        }
    }

    glm::vec3 cross = glm::cross(upPerpendicular, zaxis);
    if (angleMin != 0 && glm::length(cross) > 0) {
        stoppedAngle = angleMin;
        stoppedRotationAxis = glm::normalize(cross);
    } else {
        stoppedAngle = 0;
        stoppedRotationAxis = zaxis;
    }

    return upFace;
}

void DicePhysicsModel::positionDice(std::string const &symbol, float x, float y) {
    uint32_t faceIndex = getFaceIndexForSymbol(symbol);
    glm::vec3 zaxis = glm::vec3(0.0, 0.0, 1.0);

    position.x = x;
    position.y = y;
    position.z = maxposz;

    velocity = { 0.0, 0.0, 0.0 };

    angularVelocity.setAngularSpeed(0.0);
    angularVelocity.setSpinAxis(glm::vec3(0.0,0.0,0.0));

    glm::vec3 normalVector;
    float angle = 0;
    getAngleAxis(faceIndex, angle, normalVector);
    glm::vec3 cross = glm::cross(normalVector, zaxis);
    if (glm::length(cross) == 0.0f) {
        cross = normalVector;
    }
    if (glm::length(cross) == 0.0f) {
        cross = zaxis;
    }

    if (angle != 0) {
        glm::quat quaternian = glm::angleAxis(angle, glm::normalize(cross));
        checkQuaternion(quaternian);

        qTotalRotated = glm::normalize(quaternian);
        checkQuaternion(qTotalRotated);
    }

    glm::mat4 scale = glm::scale(glm::vec3(stoppedRadius, stoppedRadius, stoppedRadius));
    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    glm::mat4 translate = glm::translate(position);
    ubo.model = translate * rotate * scale;

    yAlign(faceIndex);
    if (stoppedAngle != 0) {
        glm::quat q = glm::angleAxis(stoppedAngle, stoppedRotationAxis);
        checkQuaternion(qTotalRotated);
        checkQuaternion(q);
        qTotalRotated = glm::normalize(q*qTotalRotated);
    }

    scale = glm::scale(glm::vec3(stoppedRadius, stoppedRadius, stoppedRadius));
    rotate = glm::toMat4(qTotalRotated);
    translate = glm::translate(position);
    ubo.model = translate * rotate * scale;
}

void DicePhysicsModel::randomizeUpFace() {
    Random random;
    uint32_t upFace = random.getUInt(0, numberFaces-1);

    float angle2 = random.getFloat(0.0f, 2 * pi);
    glm::vec3 normalVector;
    glm::vec3 zaxis = glm::vec3(0.0, 0.0, 1.0);
    float angle = 0;
    getAngleAxis(upFace, angle, normalVector);
    glm::vec3 cross = glm::cross(normalVector, zaxis);
    if (glm::length(cross) == 0.0f) {
        cross = normalVector;
    }
    if (glm::length(cross) == 0.0f) {
        cross = zaxis;
    }

    if (angle != 0) {
        glm::quat quaternian = glm::angleAxis(angle, glm::normalize(cross));
        checkQuaternion(quaternian);

        glm::quat quaternian2 = glm::angleAxis(angle2, zaxis);
        checkQuaternion(quaternian2);

        qTotalRotated = glm::normalize(quaternian2 * quaternian);
        checkQuaternion(qTotalRotated);
    }

    position.x = random.getFloat(-screenWidth/2, screenWidth/2);
    position.y = random.getFloat(-screenHeight/2, screenHeight/2);
    position.z = -maxposz;

    float factor = 10;
    velocity.x = random.getFloat(-factor, factor);
    velocity.y = random.getFloat(-factor, factor);
    velocity.x = random.getFloat(-factor, factor);
}

void DiceModelCube::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    Vertex vertex = {};

    uint32_t totalNbrImages = texAtlas->getNbrImages();
    float paddingTotal = totalNbrImages * (float)texAtlas->getPaddingHeight()/(float)texAtlas->getTextureHeight();
    glm::vec3 normals[6] = {{0.0f, 1.0f, 0.0f},
                            {0.0f, -1.0f, 0.0f},
                            {sqrtf(2) / 2, 0.0f, sqrtf(2) / 2},
                            {-sqrtf(2) / 2, 0.0f, sqrtf(2) / 2},
                            {-sqrtf(2) / 2, 0.0f, -sqrtf(2) / 2},
                            {sqrtf(2) / 2, 0.0f, -sqrtf(2) / 2} };
    // vertices
    for (uint32_t i = 0; i < 4; i ++) {
        // top (y is "up")
        vertex.normal = normals[0];
        cubeTop(vertex.corner1, 0);
        cubeTop(vertex.corner2, 3);
        cubeTop(vertex.corner3, 2);
        cubeTop(vertex.corner4, 1);
        // set to something that the shaders can check for to see if this is a valid corner.  The
        // fragment shader will check to see if the length is greater than 1000 since that is outside
        // of our display area.
        vertex.corner5 = {1000.0,1000.0,1000.0};

        uint32_t textureToUse = texAtlas->getImageIndex(symbols[0]);
        float paddingHeight = (float)texAtlas->getPaddingHeight() / (float)texAtlas->getTextureHeight()*(textureToUse);
        switch (i) {
        case 0:
            vertex.texCoord = {0, ((1.0f - paddingTotal)/ totalNbrImages) * (textureToUse + 1) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[2] + normals[5]);
            break;
        case 1:
            vertex.texCoord = {0, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[3] + normals[2]);
            break;
        case 2:
            vertex.texCoord = {1, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[4] + normals[3]);
            break;
        case 3:
        default:
            vertex.texCoord = {1, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse + 1) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[5] + normals[4]);
            break;
        }
        vertex.color = colors[i%colors.size()];
        cubeTop(vertex.pos, i);
        vertices.push_back(vertex);

        //bottom
        vertex.normal = normals[1];
        cubeBottom(vertex.corner1, 0);
        cubeBottom(vertex.corner2, 1);
        cubeBottom(vertex.corner3, 2);
        cubeBottom(vertex.corner4, 3);
        // set to something that the shaders can check for to see if this is a valid corner.  The
        // fragment shader will check to see if the length is greater than 1000 since that is outside
        // of our display area.
        vertex.corner5 = {1000.0,1000.0,1000.0};
        textureToUse = texAtlas->getImageIndex(symbols[1%symbols.size()]);
        paddingHeight = (float)texAtlas->getPaddingHeight() / (float)texAtlas->getTextureHeight()*(textureToUse);
        switch (i) {
        case 0:
            vertex.texCoord = {0, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[2] + normals[5]);
            break;
        case 1:
            vertex.texCoord = {0, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse + 1) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[3] + normals[2]);
            break;
        case 2:
            vertex.texCoord = {1, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse + 1) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[4] + normals[3]);
            break;
        case 3:
        default:
            vertex.texCoord = {1, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[5] + normals[4]);
            break;
        }
        vertex.color = colors[(i+3) %colors.size()];
        cubeBottom(vertex.pos, i);
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
        uint32_t textureToUse = texAtlas->getImageIndex(symbols[(i+2)%symbols.size()]);
        float paddingHeight = (float)texAtlas->getPaddingHeight() / (float)texAtlas->getTextureHeight() *(textureToUse);

        vertex.normal = normals[i+2];
        cubeTop(vertex.corner1, i);
        cubeTop(vertex.corner2, (i+1)%4);
        cubeBottom(vertex.corner3, (i+1)%4);
        cubeBottom(vertex.corner4, i);

        // set to something that the shaders can check for to see if this is a valid corner.  The
        // fragment shader will check to see if the length is greater than 1000 since that is outside
        // of our display area.
        vertex.corner5 = {1000.0,1000.0,1000.0};

        cubeTop(vertex.pos, i);
        vertex.texCoord = {0, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[0] + normals[(i+3)%4+2]);
        vertices.push_back(vertex);

        cubeTop(vertex.pos, (i+1)%4);
        vertex.texCoord = {0, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse + 1) + paddingHeight};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[0] + normals[(i+1)%4+2]);
        vertices.push_back(vertex);

        cubeBottom(vertex.pos, i);
        vertex.texCoord = {1, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[1] + normals[(i+3)%4+2]);
        vertices.push_back(vertex);

        cubeBottom(vertex.pos, (i+1)%4);
        vertex.texCoord = {1, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse + 1) + paddingHeight};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[1] + normals[(i+1)%4+2]);
        vertices.push_back(vertex);

        indices.push_back(9+4*i);
        indices.push_back(11+4*i);
        indices.push_back(8+4*i);

        indices.push_back(8+4*i);
        indices.push_back(11+4*i);
        indices.push_back(10+4*i);
    }
}

void DiceModelCube::cubeTop(glm::vec3 &pos, uint32_t i) {
    pos = {glm::cos(2*i*pi/4), sqrtf(2)/2, glm::sin(2*i*pi/4)};
}

void DiceModelCube::cubeBottom(glm::vec3 &pos, uint32_t i) {
    pos = {glm::cos(2*i*pi/4), -sqrtf(2)/2, glm::sin(2*i*pi/4)};
}

void DiceModelCube::getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis) {
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);
    glm::vec3 upPerpendicular;

    if (faceIndex == 0) {
        /* what was the top face at the start (y being up) */
        glm::vec4 p04 = ubo.model * glm::vec4(vertices[2].pos, 1.0f);
        glm::vec4 q4 = ubo.model * glm::vec4(vertices[0].pos, 1.0f);
        glm::vec4 r4 = ubo.model * glm::vec4(vertices[6].pos, 1.0f);
        glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
        glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
        glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
        axis = glm::normalize(glm::cross(q - r, q - p0));
        angle = glm::acos(glm::dot(axis, zaxis));
    } else if (faceIndex == 1) {
        /* what was the bottom face at the start */
        glm::vec4 p04 = ubo.model * glm::vec4(vertices[7].pos, 1.0f);
        glm::vec4 q4 = ubo.model * glm::vec4(vertices[1].pos, 1.0f);
        glm::vec4 r4 = ubo.model * glm::vec4(vertices[3].pos, 1.0f);
        glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
        glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
        glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
        axis = glm::normalize(glm::cross(q - r, q - p0));
        angle = glm::acos(glm::dot(axis, zaxis));
    } else {
        /* what were the side faces at the start */
        glm::vec4 p04 = ubo.model * glm::vec4(vertices[4 * faceIndex].pos, 1.0f);
        glm::vec4 q4 = ubo.model * glm::vec4(vertices[1 + 4 * faceIndex].pos, 1.0f);
        glm::vec4 r4 = ubo.model * glm::vec4(vertices[3 + 4 * faceIndex].pos, 1.0f);
        glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
        glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
        glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
        axis = glm::normalize(glm::cross(q - r, q - p0));
        angle = glm::acos(glm::dot(axis, zaxis));
    }
}

void DiceModelCube::yAlign(uint32_t faceIndex) {
    glm::vec3 yaxis;
    glm::vec3 zaxis;
    if (isOpenGl) {
        zaxis = {0.0f, 0.0f, -1.0f};
        yaxis = {0.0f, -1.0f, 0.0f};
    } else {
        zaxis = {0.0f, 0.0f, 1.0f};
        yaxis = {0.0f, 1.0f, 0.0f};
    }
    glm::vec3 axis;
    if (faceIndex == 0 || faceIndex == 1) {
        glm::vec4 p24 = ubo.model * glm::vec4(vertices[2].pos, 1.0f);
        glm::vec4 p04 = ubo.model * glm::vec4(vertices[0].pos, 1.0f);
        glm::vec3 p2 = {p24.x, p24.y, p24.z};
        glm::vec3 p0 = {p04.x, p04.y, p04.z};
        axis = p2 - p0;
        if (faceIndex == 1) {
            axis = -axis;
        }
    } else {
        glm::vec4 p84 = ubo.model * glm::vec4(vertices[8+4*(faceIndex-2)].pos, 1.0f);
        glm::vec4 p94 = ubo.model * glm::vec4(vertices[9+4*(faceIndex-2)].pos, 1.0f);
        glm::vec3 p9 = {p94.x, p94.y, p94.z};
        glm::vec3 p8 = {p84.x, p84.y, p84.z};
        axis = p8 - p9;
    }

    float angle = glm::acos(glm::dot(glm::normalize(axis), yaxis));
    if (axis.x < 0) {
        angle = -angle;
    }

    stoppedRotationAxis = zaxis;
    stoppedAngle = angle;
}

void DiceModelHedron::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    glm::vec3 p0TopCornerNormal = {};
    glm::vec3 p0BottomCornerNormal = {};
    for (uint32_t i = 0; i < numberFaces/2; i ++) {
        glm::vec3 p0, q, r;
        topCorners(p0,q,r,i);
        p0TopCornerNormal += glm::normalize(glm::cross(r-q, p0-q));

        bottomCorners(p0,q,r,i);
        p0BottomCornerNormal += glm::normalize(glm::cross(r-q, p0-q));
    }

    p0TopCornerNormal = glm::normalize(p0TopCornerNormal);
    p0BottomCornerNormal = glm::normalize(p0BottomCornerNormal);

    for (uint32_t i = 0; i < numberFaces/2; i ++) {
        glm::vec3 p0, q, r;
        glm::vec3 qNormal = {};
        glm::vec3 rNormal = {};

        // corner normal for q(top)/r(bottom)
        if (i==0) {
            topCorners(p0,q,r,numberFaces/2-1);
            qNormal += glm::normalize(glm::cross(r-q, p0-q));
            bottomCorners(p0,q,r,numberFaces/2-1);
            qNormal += glm::normalize(glm::cross(r-q, p0-q));
        } else {
            topCorners(p0, q, r, i-1);
            qNormal += glm::normalize(glm::cross(r-q, p0-q));
            bottomCorners(p0, q, r, i-1);
            qNormal += glm::normalize(glm::cross(r-q, p0-q));
        }

        topCorners(p0, q, r, i);
        qNormal += glm::normalize(glm::cross(r-q, p0-q));
        bottomCorners(p0, q, r, i);
        qNormal += glm::normalize(glm::cross(r-q, p0-q));
        qNormal = glm::normalize(qNormal);

        // corner normal for r(top)/q(bottom)
        topCorners(p0, q, r, i);
        rNormal += glm::normalize(glm::cross(r-q, p0-q));
        bottomCorners(p0, q, r, i);
        rNormal += glm::normalize(glm::cross(r-q, p0-q));
        topCorners(p0, q, r, i+1);
        rNormal += glm::normalize(glm::cross(r-q, p0-q));
        bottomCorners(p0, q, r, i+1);
        rNormal += glm::normalize(glm::cross(r-q, p0-q));
        rNormal = glm::normalize(rNormal);

        // bottom
        bottomCorners(p0, q, r, i);
        addVertices(texAtlas, p0, q, r, p0BottomCornerNormal, rNormal, qNormal, i);

        // top
        topCorners(p0, q, r, i);
        addVertices(texAtlas, p0, q, r, p0TopCornerNormal, qNormal, rNormal, i+numberFaces/2);
    }

    // indices - not really using these
    for (uint32_t i = 0; i < numberFaces*15; i ++) {
        indices.push_back(i);
    }
}

void DiceModelHedron::topCorners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, int i) {
    p0 = {0.0f, 1.0f, 0.0f};
    q = {glm::cos(4.0f*((i+1)%(numberFaces/2))*pi/numberFaces), 0.0f, glm::sin(4*((i+1)%(numberFaces/2))*pi/numberFaces)};
    r = {glm::cos(4*i*pi/numberFaces), 0.0f, glm::sin(4*i*pi/numberFaces)};
}

void DiceModelHedron::bottomCorners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, int i) {
    p0 = {0.0f, -1.0f, 0.0f};
    q = {glm::cos(4*i*pi/numberFaces), 0.0f, glm::sin(4*i*pi/numberFaces)};
    r = {glm::cos(4.0f*((i+1)%(numberFaces/2))*pi/numberFaces), 0.0f, glm::sin(4*((i+1)%(numberFaces/2))*pi/numberFaces)};
}

void DiceModelHedron::addVertices(std::shared_ptr<TextureAtlas> const &texAtlas,
                                  glm::vec3 const &p0, glm::vec3 const &q, glm::vec3 const &r,
                                  glm::vec3 const &p0Normal, glm::vec3 const &qNormal, glm::vec3 const &rNormal,
                                  uint32_t i) {
    float textureToUse = texAtlas->getImageIndex(symbols[i%symbols.size()]);
    float totalNbrImages = texAtlas->getNbrImages();
    float a = (float)texAtlas->getImageWidth()/(float)texAtlas->getImageHeight();
    float heightPadding = (float)texAtlas->getPaddingHeight()/(float)texAtlas->getTextureHeight();
    float paddingTotal = totalNbrImages * heightPadding;

    Vertex vertex = {};

    float k = a * glm::length((r+q)/2.0f-p0) / glm::length(r-q);

    glm::vec3 p2 = (1.0f - 1.0f/(2.0f * k + 2.0f)) * r + (1.0f/(2.0f*k+2.0f)) * q;
    glm::vec3 p3 = 1.0f/(2.0f*k+2.0f) * r + (1.0f-1.0f/(2.0f*k+2.0f)) * q;
    glm::vec3 p1 = p2 - 1.0f/(1.0f+k) * ((r+q)/2.0f - p0);
    glm::vec3 p1prime = p3 - 1.0f/(1.0f+k) * ((r+q)/2.0f - p0);

    // the vector normal to this face
    vertex.normal = glm::normalize(glm::cross(r-q, p0-q));

    vertex.corner1 = p0;
    vertex.corner2 = q;
    vertex.corner3 = r;
    vertex.corner4 = {1000.0, 1000.0, 1000.0};
    vertex.corner5 = {1000.0, 1000.0, 1000.0};

    // Top point
    vertex.pos = p0;
    vertex.cornerNormal = p0Normal;
    vertex.texCoord = {0.5 * glm::length(p2-p3),
        ((textureToUse - glm::length(0.5f*(r+q) - p0) + glm::length(p1-p2))*(1.0f-paddingTotal)/totalNbrImages)+heightPadding*textureToUse};
    vertex.color = colors[i%colors.size()];
    vertices.push_back(vertex);

    // left point
    vertex.pos = q;
    vertex.cornerNormal = qNormal;
    vertex.texCoord = {0.0f - glm::length(q-p3)/glm::length(p2-p3),
                       (textureToUse + 1)*(1.0f - paddingTotal)/totalNbrImages+heightPadding*textureToUse};
    vertex.color = colors[(i+2) %colors.size()];
    vertices.push_back(vertex);

    // right point
    vertex.pos = r;
    vertex.cornerNormal = rNormal;
    vertex.texCoord = {1.0f + glm::length(r-p2)/glm::length(p2-p3),
                       (textureToUse+1)*(1.0f-paddingTotal)/totalNbrImages +heightPadding*textureToUse};
    vertex.color = colors[(i+2)%colors.size()];
    vertices.push_back(vertex);
}

void DiceModelHedron::getAngleAxis(uint32_t face, float &angle, glm::vec3 &axis) {
    const uint32_t nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);
    glm::vec4 p04 = ubo.model * glm::vec4(vertices[face * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 q4 = ubo.model * glm::vec4(vertices[1 + face * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 r4 = ubo.model * glm::vec4(vertices[2 + face * nbrVerticesPerFace].pos, 1.0f);
    glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
    glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
    glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
    axis = glm::normalize(glm::cross(q-r, q-p0));
    angle = glm::acos(glm::dot(axis, zaxis));
}

uint32_t DiceModelHedron::getUpFaceIndex(uint32_t i) {
    if (i%2 == 0) {
        return i/2;
    } else {
        return i/2 + numberFaces/2;
    }
}

uint32_t DiceModelHedron::getFaceIndexForSymbol(std::string symbol) {
    uint32_t i=0;
    for (auto &&s : symbols) {
        if (symbol == s) {
            if (i >= numberFaces/2) {
                return (i%(numberFaces/2)) * 2 + 1;
            } else {
                return i*2;
            }
            return i;
        }
        i++;
    }
    return 0;  // shouldn't be reached.
}

void DiceModelHedron::yAlign(uint32_t faceIndex) {
    const uint32_t nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 yaxis;
    glm::vec3 zaxis;
    if (isOpenGl) {
        zaxis = {0.0f, 0.0f, -1.0f};
        yaxis = {0.0f, -1.0f, 0.0f};
    } else {
        zaxis = {0.0f, 0.0f, 1.0f};
        yaxis = {0.0f, 1.0f, 0.0f};
    }
    glm::vec3 axis;

    glm::vec4 p04 = ubo.model * glm::vec4(vertices[0 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 q4 = ubo.model * glm::vec4(vertices[1 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 r4 = ubo.model * glm::vec4(vertices[2 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec3 p0 = {p04.x, p04.y, p04.z};
    glm::vec3 q = {q4.x, q4.y, q4.z};
    glm::vec3 r = {r4.x, r4.y, r4.z};
    axis = p0 - (0.5f * (r + q));

    float angle = glm::acos(glm::dot(glm::normalize(axis), yaxis));
    if (axis.x < 0) {
        angle = -angle;
    }
    stoppedRotationAxis = zaxis;
    stoppedAngle = angle;
}

void DiceModelTetrahedron::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    glm::vec3 p0Normal = {};
    glm::vec3 qNormal = {};
    glm::vec3 rNormal = {};

    glm::vec3 p0, r, q;
    corners(p0,q,r,0);
    p0Normal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,1);
    p0Normal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,2);
    p0Normal += glm::normalize(glm::cross(r-q, p0-q));
    p0Normal = glm::normalize(p0Normal);

    corners(p0,q,r,0);
    qNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,3);
    qNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,2);
    qNormal += glm::normalize(glm::cross(r-q, p0-q));
    qNormal = glm::normalize(qNormal);

    corners(p0,q,r,0);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,1);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,3);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    rNormal = glm::normalize(rNormal);

    corners(p0,q,r,0);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, 0);

    // p0Normal stays the same, r becomes q.
    qNormal = rNormal;
    rNormal = {};
    corners(p0,q,r,1);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,2);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,3);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    rNormal = glm::normalize(rNormal);

    corners(p0,q,r,1);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, 1);

    // p0Normal stays the same, r becomes q.
    qNormal = rNormal;
    rNormal = {};
    corners(p0,q,r,0);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,2);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,3);
    rNormal += glm::normalize(glm::cross(r-q, p0-q));
    rNormal = glm::normalize(rNormal);

    corners(p0,q,r,2);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, 2);

    // rNormal stays the same, q becomes p0.
    p0Normal = qNormal;
    qNormal = {};
    corners(p0,q,r,0);
    qNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,1);
    qNormal += glm::normalize(glm::cross(r-q, p0-q));
    corners(p0,q,r,3);
    qNormal += glm::normalize(glm::cross(r-q, p0-q));
    qNormal = glm::normalize(qNormal);

    corners(p0,q,r,3);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, 3);

    // indices - not really using these
    for (uint32_t i = 0; i < numberFaces*15; i ++) {
        indices.push_back(i);
    }
}

void DiceModelTetrahedron::corners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, uint32_t i) {
    if (i==0) {
        p0 = {0.0f, 1.0f, 1.0f / sqrtf(2)};
        q = {0.0f, -1.0f, 1.0f / sqrtf(2)};
        r = {1.0f, 0.0f, -1.0f / sqrtf(2)};
    } else if (i==1) {
        p0 = {0.0f, 1.0f, 1.0f / sqrtf(2)};
        q = {-1.0f, 0.0f, -1.0f / sqrtf(2)};
        r = {0.0f, -1.0f, 1.0f / sqrtf(2)};
    } else if (i==2) {
        p0 = {0.0f, 1.0f, 1.0f / sqrtf(2)};
        q = {1.0f, 0.0f, -1.0f / sqrtf(2)};
        r = {-1.0f, 0.0f, -1.0f / sqrtf(2)};
    } else { // i == 3
        p0 = {-1.0f, 0.0f, -1.0f / sqrtf(2)};
        q = {1.0f, 0.0f, -1.0f / sqrtf(2)};
        r = {0.0f, -1.0f, 1.0f / sqrtf(2)};
    }
}

void DiceModelIcosahedron::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    uint32_t i = 0;
    glm::vec3 p0, q, r;
    glm::vec3 p0Normal = {};
    glm::vec3 qNormal = {};
    glm::vec3 rNormal = {};

    glm::vec3 faceNormals[20];
    // get all of the face normals.
    for (uint32_t j = 0; j < 20; j++) {
        corners(p0,q,r,j);
        faceNormals[j] = glm::normalize(glm::cross(r-q, p0-q));
    }

    // we'll divide it into top middle and bottom.  First the top:
    p0Normal = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[2] + faceNormals[3] + faceNormals[4]);

    qNormal = glm::normalize(faceNormals[0] + faceNormals[4] + faceNormals[12] + faceNormals[13] + faceNormals[14]);
    rNormal = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[10] + faceNormals[11] + faceNormals[12]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[2] + faceNormals[3] + faceNormals[16] + faceNormals[17] + faceNormals[18]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[14] + faceNormals[15] + faceNormals[16]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[0] + faceNormals[4] + faceNormals[12] + faceNormals[13] + faceNormals[14]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    // now for the bottom
    // the normal for p0.
    p0Normal = glm::normalize(faceNormals[5] + faceNormals[6] + faceNormals[7] + faceNormals[8] + faceNormals[9]);

    qNormal = glm::normalize(faceNormals[5] + faceNormals[9] + faceNormals[15] + faceNormals[16] + faceNormals[17]);
    rNormal = glm::normalize(faceNormals[5] + faceNormals[6] + faceNormals[17] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[6] + faceNormals[7] + faceNormals[10] + faceNormals[11] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[7] + faceNormals[8] + faceNormals[11] + faceNormals[12] + faceNormals[13]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[8] + faceNormals[9] + faceNormals[13] + faceNormals[14] + faceNormals[15]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[5] + faceNormals[9] + faceNormals[15] + faceNormals[16] + faceNormals[17]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    // now the middle
    p0Normal = glm::normalize(faceNormals[6] + faceNormals[7] + faceNormals[10] + faceNormals[11] + faceNormals[19]);
    qNormal = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10] + faceNormals[18] + faceNormals[19]);
    rNormal = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[10] + faceNormals[11] + faceNormals[12]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[7] + faceNormals[8] + faceNormals[11] + faceNormals[12] + faceNormals[13]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[0] + faceNormals[4] + faceNormals[12] + faceNormals[13] + faceNormals[14]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[8] + faceNormals[9] + faceNormals[13] + faceNormals[14] + faceNormals[15]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[14] + faceNormals[15] + faceNormals[16]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[5] + faceNormals[9] + faceNormals[15] + faceNormals[16] + faceNormals[17]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[2] + faceNormals[3] + faceNormals[16] + faceNormals[17] + faceNormals[18]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[5] + faceNormals[6] + faceNormals[17] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[6] + faceNormals[7] + faceNormals[10] + faceNormals[11] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i++);

    // indices - not really using these
    for (i = 0; i < numberFaces*15; i ++) {
        indices.push_back(i);
    }
}

void DiceModelIcosahedron::corners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, uint32_t i) {
    float phi = (1+sqrtf(5.0f))/2;
    float scaleFactor = 1.8f;

    // we'll divide it into top middle and bottom.  First the top:
    p0 = {0.0f, 1.0f/scaleFactor, phi/scaleFactor};
    q = {1.0f/scaleFactor, phi/scaleFactor, 0.0f};
    r = {-1.0f/scaleFactor, phi/scaleFactor, 0.0f};
    if (i == 0) {
        return;
    }

    q = r;
    r = {-phi/scaleFactor, 0.0f, 1.0f/scaleFactor};
    if (i == 1) {
        return;
    }

    q = r;
    r = {0.0f, -1.0f/scaleFactor, phi/scaleFactor};
    if (i == 2) {
        return;
    }

    q = r;
    r = {phi/scaleFactor, 0.0f, 1.0f/scaleFactor};
    if (i == 3) {
        return;
    }

    q = r;
    r = {1.0f/scaleFactor, phi/scaleFactor, 0.0f};
    if (i == 4) {
        return;
    }

    // now for the bottom
    p0 = {0.0f, -1.0f/scaleFactor, -phi/scaleFactor};
    q = {1.0f/scaleFactor, -phi/scaleFactor, 0.0f};
    r = {-1.0f/scaleFactor, -phi/scaleFactor, 0.0f};
    if (i == 5) {
        return;
    }

    q = r;
    r = {-phi/scaleFactor, 0.0f, -1.0f/scaleFactor};
    if (i == 6) {
        return;
    }

    q = r;
    r = {0.0f, 1.0f/scaleFactor, -phi/scaleFactor};
    if (i == 7) {
        return;
    }

    q = r;
    r = {phi/scaleFactor, 0.0f, -1.0f/scaleFactor};
    if (i == 8) {
        return;
    }

    q = r;
    r = {1.0f/scaleFactor, -phi/scaleFactor, 0.0f};
    if (i == 9) {
        return;
    }

    // now the middle
    p0 = {-phi/scaleFactor, 0.0f, -1.0f/scaleFactor};
    q = {-phi/scaleFactor, 0.0f, 1.0f/scaleFactor};
    r = {-1.0f/scaleFactor, phi/scaleFactor, 0.0f};
    if (i == 10) {
        return;
    }

    q = r;
    r = p0;
    p0 = q;
    q = {0.0f, 1.0f/scaleFactor, -phi/scaleFactor};
    if (i == 11) {
        return;
    }

    r = q;
    q = p0;
    p0 = r;
    r = {1.0f/scaleFactor, phi/scaleFactor, 0.0f};
    if (i == 12) {
        return;
    }

    p0 = {1.0f/scaleFactor, phi/scaleFactor, 0.0f};
    q =  {phi/scaleFactor, 0.0f, -1.0f/scaleFactor};
    r = {0.0f, 1.0f/scaleFactor, -phi/scaleFactor};
    if (i == 13) {
        return;
    }

    r = q;
    q = p0;
    p0 = r;
    r = {phi/scaleFactor, 0.0f, 1.0f/scaleFactor};
    if (i == 14) {
        return;
    }

    q = r;
    r = p0;
    p0 = q;
    q =  {1.0f/scaleFactor, -phi/scaleFactor, 0.0f};
    if (i == 15) {
        return;
    }

    r = q;
    q = p0;
    p0 = r;
    r = {0.0f, -1.0f/scaleFactor, phi/scaleFactor};
    if (i == 16) {
        return;
    }

    p0 = {0.0f, -1.0f/scaleFactor, phi/scaleFactor};
    r = {1.0f/scaleFactor, -phi/scaleFactor, 0.0f};
    q =  {-1.0f/scaleFactor, -phi/scaleFactor, 0.0f};
    if (i == 17) {
        return;
    }

    r = q;
    q = p0;
    p0 = r;
    r = {-phi/scaleFactor, 0.0f, 1.0f/scaleFactor};
    if (i == 18) {
        return;
    }

    q = r;
    r = p0;
    p0 = q;
    q =  {-phi/scaleFactor, 0.0f, -1.0f/scaleFactor};
    // i == 19
}

void DiceModelDodecahedron::addVertices(std::shared_ptr<TextureAtlas> const &texAtlas,
                                        glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 e,
                                        glm::vec3 cornerNormalA, glm::vec3 cornerNormalB, glm::vec3 cornerNormalC,
                                        glm::vec3 cornerNormalD, glm::vec3 cornerNormalE, uint32_t i) {
    uint32_t totalNbrImages = texAtlas->getNbrImages();
    float paddingTotal = totalNbrImages * (float)texAtlas->getPaddingHeight()/(float)texAtlas->getTextureHeight();

    uint32_t textureToUse = texAtlas->getImageIndex(symbols[i % symbols.size()]);
    float paddingHeight = (float)texAtlas->getPaddingHeight() / (float)texAtlas->getTextureHeight()*(textureToUse);

    Vertex vertex = {};

    glm::vec3 p1 = 1.0f/2.0f * (e+b-d+c);
    glm::vec3 p2 = 1.0f/2.0f * (e+b+d-c);

    // normal vector to the face
    vertex.normal = glm::normalize(glm::cross(d-c, b-c));
    vertex.corner1 = a;
    vertex.corner2 = b;
    vertex.corner3 = c;
    vertex.corner4 = d;
    vertex.corner5 = e;

    // Top triangle
    // not really using textures for this triangle
    vertex.pos = a;
    vertex.cornerNormal = cornerNormalA;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[i % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = b;
    vertex.cornerNormal = cornerNormalB;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = e;
    vertex.cornerNormal = cornerNormalE;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    // right bottom triangle
    // not really using textures for this triangle
    vertex.pos = b;
    vertex.cornerNormal = cornerNormalB;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = c;
    vertex.cornerNormal = cornerNormalC;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 2) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = p1;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    // left bottom triangle
    // not really using textures for this triangle
    vertex.pos = e;
    vertex.cornerNormal = cornerNormalE;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = d;
    vertex.cornerNormal = cornerNormalD;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = colors[(i + 2) % colors.size()];
    vertices.push_back(vertex);

    // bottom texture triangle
    vertex.pos = p1;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {0.0f, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = c;
    vertex.cornerNormal = cornerNormalC;
    vertex.texCoord = {0.0f, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse+1) + paddingHeight};
    vertex.color = colors[(i + 2) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = d;
    vertex.cornerNormal = cornerNormalD;
    vertex.texCoord = {1.0f, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse+1) + paddingHeight};
    vertex.color = colors[(i + 2) % colors.size()];
    vertices.push_back(vertex);

    // top texture triangle
    vertex.pos = p1;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {0.0f, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = d;
    vertex.cornerNormal = cornerNormalD;
    vertex.texCoord = {1.0f, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse+1) + paddingHeight};
    vertex.color = colors[(i + 2) % colors.size()];
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {1.0f, ((1.0f - paddingTotal) / totalNbrImages) * (textureToUse) + paddingHeight};
    vertex.color = colors[(i + 1) % colors.size()];
    vertices.push_back(vertex);
}

void DiceModelDodecahedron::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    uint32_t i = 0;
    float scaleFactor = 2.0f;
    glm::vec3 a,b,c,d,e;
    glm::vec3 cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE;
    glm::vec3 faceNormals[12];

    // get all the face normal vectors.
    for (uint32_t j = 0; j < 12; j++) {
        corners(a,b,c,d,e,j);
        faceNormals[j] = glm::normalize(glm::cross(d-c, b-c));
    }

    // one side
    corners(a,b,c,d,e,i);
    cornerNormalA = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[2]);
    cornerNormalB = glm::normalize(faceNormals[0] + faceNormals[2] + faceNormals[6]);
    cornerNormalC = glm::normalize(faceNormals[0] + faceNormals[6] + faceNormals[7]);
    cornerNormalD = glm::normalize(faceNormals[0] + faceNormals[7] + faceNormals[8]);
    cornerNormalE = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[8]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[8]);
    cornerNormalC = glm::normalize(faceNormals[1] + faceNormals[8] + faceNormals[9]);
    cornerNormalD = glm::normalize(faceNormals[1] + faceNormals[9] + faceNormals[10]);
    cornerNormalE = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10]);
    cornerNormalC = glm::normalize(faceNormals[2] + faceNormals[10] + faceNormals[11]);
    cornerNormalD = glm::normalize(faceNormals[2] + faceNormals[6] + faceNormals[11]);
    cornerNormalE = glm::normalize(faceNormals[0] + faceNormals[2] + faceNormals[6]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    // the other side
    corners(a,b,c,d,e,i);
    cornerNormalA = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[5]);
    cornerNormalB = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[11]);
    cornerNormalC = glm::normalize(faceNormals[3] + faceNormals[10] + faceNormals[11]);
    cornerNormalD = glm::normalize(faceNormals[3] + faceNormals[9] + faceNormals[10]);
    cornerNormalE = glm::normalize(faceNormals[3] + faceNormals[5] + faceNormals[9]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = glm::normalize(faceNormals[4] + faceNormals[5] + faceNormals[7]);
    cornerNormalC = glm::normalize(faceNormals[4] + faceNormals[6] + faceNormals[7]);
    cornerNormalD = glm::normalize(faceNormals[4] + faceNormals[6] + faceNormals[11]);
    cornerNormalE = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[11]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = glm::normalize(faceNormals[3] + faceNormals[5] + faceNormals[9]);
    cornerNormalC = glm::normalize(faceNormals[5] + faceNormals[8] + faceNormals[9]);
    cornerNormalD = glm::normalize(faceNormals[5] + faceNormals[7] + faceNormals[8]);
    cornerNormalE = glm::normalize(faceNormals[4] + faceNormals[5] + faceNormals[7]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    // the middle
    corners(a,b,c,d,e,i);
    cornerNormalA = glm::normalize(faceNormals[0] + faceNormals[2] + faceNormals[6]);
    cornerNormalB = glm::normalize(faceNormals[2] + faceNormals[6] + faceNormals[11]);
    cornerNormalC = glm::normalize(faceNormals[4] + faceNormals[6] + faceNormals[11]);
    cornerNormalD = glm::normalize(faceNormals[4] + faceNormals[6] + faceNormals[7]);
    cornerNormalE = glm::normalize(faceNormals[0] + faceNormals[6] + faceNormals[7]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = cornerNormalE;
    cornerNormalC = cornerNormalD;
    cornerNormalA = glm::normalize(faceNormals[0] + faceNormals[7] + faceNormals[8]);
    cornerNormalD = glm::normalize(faceNormals[4] + faceNormals[5] + faceNormals[7]);
    cornerNormalE = glm::normalize(faceNormals[5] + faceNormals[7] + faceNormals[8]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = cornerNormalA;
    cornerNormalC = cornerNormalE;
    cornerNormalA = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[8]);
    cornerNormalD = glm::normalize(faceNormals[5] + faceNormals[8] + faceNormals[9]);
    cornerNormalE = glm::normalize(faceNormals[1] + faceNormals[8] + faceNormals[9]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = cornerNormalE;
    cornerNormalC = cornerNormalD;
    cornerNormalA = glm::normalize(faceNormals[1] + faceNormals[9] + faceNormals[10]);
    cornerNormalD = glm::normalize(faceNormals[3] + faceNormals[5] + faceNormals[9]);
    cornerNormalE = glm::normalize(faceNormals[3] + faceNormals[9] + faceNormals[10]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = cornerNormalA;
    cornerNormalC = cornerNormalE;
    cornerNormalA = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10]);
    cornerNormalD = glm::normalize(faceNormals[3] + faceNormals[10] + faceNormals[11]);
    cornerNormalE = glm::normalize(faceNormals[2] + faceNormals[10] + faceNormals[11]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    corners(a,b,c,d,e,i);
    cornerNormalB = cornerNormalE;
    cornerNormalC = cornerNormalD;
    cornerNormalA = glm::normalize(faceNormals[2] + faceNormals[6] + faceNormals[11]);
    cornerNormalD = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[11]);
    cornerNormalE = glm::normalize(faceNormals[4] + faceNormals[6] + faceNormals[11]);
    addVertices(texAtlas, a/scaleFactor, b/scaleFactor, c/scaleFactor, d/scaleFactor, e/scaleFactor,
                cornerNormalA, cornerNormalB, cornerNormalC, cornerNormalD, cornerNormalE, i++);

    // indices - not really using these
    for (i = 0; i < numberFaces*5*3; i ++) {
        indices.push_back(i);
    }
}

void DiceModelDodecahedron::corners(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c, glm::vec3 &d, glm::vec3 &e, uint32_t i) {
    float phi = (1+sqrtf(5.0f))/2;

    // one side
    a = {-1.0f, 1.0f, 1.0f};
    b = {-phi, 1.0f/phi, 0.0f};
    c = {-phi, -1.0f/phi, 0.0f};
    d = {-1.0f, -1.0f, 1.0f};
    e = {-1.0f/phi, 0.0f, phi};

    if (i==0) {
        return;
    }

    b = e;
    c = {1.0/phi, 0.0f, phi};
    d = {1.0f, 1.0f, 1.0f};
    e = {0.0f, phi, 1.0f/phi};

    if (i==1) {
        return;
    }

    b = e;
    c = {0.0f, phi, -1.0f/phi};
    d = {-1.0f, 1.0f, -1.0f};
    e = {-phi, 1.0f/phi, 0.0f};

    if (i==2) {
        return;
    }

    // the other side
    a = {1.0f, -1.0f, -1.0f};
    e = {phi, -1.0f/phi, 0.0f};
    d = {phi, 1.0f/phi, 0.0f};
    c = {1.0f, 1.0f, -1.0f};
    b = {1.0f/phi, 0.0f, -phi};

    if (i==3) {
        return;
    }

    e = b;
    d = {-1.0/phi, 0.0f, -phi};
    c = {-1.0f, -1.0f, -1.0f};
    b = {0.0f, -phi, -1.0f/phi};

    if (i==4) {
        return;
    }

    e = b;
    d = {0.0f, -phi, 1.0f/phi};
    c = {1.0f, -1.0f, 1.0f};
    b = {phi, -1.0f/phi, 0.0f};

    if (i==5) {
        return;
    }

    // the middle
    a = {-phi, 1.0f/phi, 0.0f};
    b = {-1.0f, 1.0f, -1.0f};
    c = {-1.0f/phi, 0.0f, -phi};
    d = {-1.0f, -1.0f, -1.0f};
    e = {-phi, -1.0f/phi, 0.0f};

    if (i==6) {
        return;
    }

    b = e;
    c = d;
    a = {-1.0f, -1.0f, 1.0f};
    d = {0.0f, -phi, -1.0f/phi};
    e = {0.0f, -phi, 1.0f/phi};

    if (i==7) {
        return;
    }

    b = a;
    c = e;
    a = {-1.0f/phi, 0.0f, phi};
    d = {1.0f, -1.0f, 1.0f};
    e = {1.0f/phi, 0.0f, phi};

    if (i==8) {
        return;
    }

    b = e;
    c = d;
    a = {1.0f, 1.0f, 1.0f};
    d = {phi, -1.0f/phi, 0.0f};
    e = {phi, 1.0f/phi, 0.0f};

    if (i==9) {
        return;
    }

    b = a;
    c = e;
    a = {0.0f, phi, 1.0f/phi};
    d = {1.0f, 1.0f, -1.0f};
    e = {0.0f, phi, -1.0f/phi};

    if (i==10) {
        return;
    }

    b = e;
    c = d;
    a = {-1.0f, 1.0f, -1.0f};
    d = {1.0/phi, 0.0f, -phi};
    e = {-1.0/phi, 0.0f, -phi};
}

void DiceModelDodecahedron::getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis) {
    uint32_t const nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);

    glm::vec4 a4 = ubo.model * glm::vec4(vertices[faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 b4 = ubo.model * glm::vec4(vertices[1 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 c4 = ubo.model * glm::vec4(vertices[4 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec3 a = glm::vec3(a4.x, a4.y, a4.z);
    glm::vec3 b = glm::vec3(b4.x, b4.y, b4.z);
    glm::vec3 c = glm::vec3(c4.x, c4.y, c4.z);
    axis = glm::normalize(glm::cross(c-b, a-b));
    angle = glm::acos(glm::dot(axis, zaxis));
}

void DiceModelDodecahedron::yAlign(uint32_t faceIndex){
    const uint32_t nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 yaxis;
    glm::vec3 zaxis;
    if (isOpenGl) {
        zaxis = {0.0f, 0.0f, -1.0f};
        yaxis = {0.0f, -1.0f, 0.0f};
    } else {
        zaxis = {0.0f, 0.0f, 1.0f};
        yaxis = {0.0f, 1.0f, 0.0f};
    }
    glm::vec3 axis;

    glm::vec4 p14 = ubo.model * glm::vec4(vertices[5 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 c4 = ubo.model * glm::vec4(vertices[4 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec3 p1 = {p14.x, p14.y, p14.z};
    glm::vec3 c = {c4.x, c4.y, c4.z};
    axis = p1 - c;

    float angle = glm::acos(glm::dot(glm::normalize(axis), yaxis));
    if (axis.x < 0) {
        angle = -angle;
    }
    stoppedRotationAxis = zaxis;
    stoppedAngle = angle;
}