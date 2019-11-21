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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <limits>
#include <chrono>
#include <vector>
#include <string>

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

const float Vertex::MODE_EDGE_DISTANCE = 0.0f;
const float Vertex::MODE_CENTER_DISTANCE = 1.0f;

float DicePhysicsModel::M_maxposx = 1.0f;
float DicePhysicsModel::M_maxposy = 1.0f;
float DicePhysicsModel::M_maxposz = 1.0f;
bool DicePhysicsModel::M_reverseGravity = false;
glm::vec3 DicePhysicsModel::acceleration{0.0f, 0.0f, 9.8f};

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

float const AngularVelocity::maxAngularSpeed = 10.0f;
float const pi = glm::acos(-1.0f);

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

std::shared_ptr<DicePhysicsModel> DicePhysicsModel::createDice(std::vector<std::string> const &symbols,
        std::vector<float> const &color, bool isOpenGL) {
    std::shared_ptr<DicePhysicsModel> die;
    glm::vec3 position(0.0f, 0.0f, -1.0f);
    long nbrSides = symbols.size();
    if (nbrSides == 2) {
        die.reset(new DiceModelCoin(symbols, position, color, isOpenGL));
    } else if (nbrSides == 4 && M_reverseGravity) {
        die.reset(new DiceModelTetrahedron(symbols, position, color, isOpenGL));
    } else if (nbrSides == 4 && !M_reverseGravity) {
        // Use the octahedron model for the four sided dice if we are not reversing gravity
        // so that we don't need to do something with the texture like put all textures on all the
        // faces like real life 4 sided die have.  It would make the textures hard to read for
        // anything other than numbers.
        die.reset(new DiceModelHedron(symbols, position, color, 8, isOpenGL));
    } else if (nbrSides == 12) {
        die.reset(new DiceModelDodecahedron(symbols, position, color, isOpenGL));
    } else if (nbrSides == 20) {
        die.reset(new DiceModelIcosahedron(symbols, position, color, isOpenGL));
    } else if (nbrSides == 30) {
        die.reset(new DiceModelRhombicTriacontahedron(symbols, position, color, isOpenGL));
    } else if (6 % nbrSides == 0) {
        die.reset(new DiceModelCube(symbols, position, color, isOpenGL));
    } else {
        die.reset(new DiceModelHedron(symbols, position, color, 0, isOpenGL));
    }

    return die;
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
    m_position = glm::vec3();
    velocity = glm::vec3();
    qTotalRotated = glm::quat();
    acceleration = glm::vec3();
    angularVelocity.setAngularSpeed(0);
    angularVelocity.setSpinAxis(glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, radius));
    m_model = scale;

    randomizeUpFace();
    checkQuaternion(qTotalRotated);

    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_position);
    m_model = translate * rotate * scale;
}

void DicePhysicsModel::calculateBounce(DicePhysicsModel *other) {
    float length = glm::length(m_position - other->m_position);
    if (length < 2 * radius) {
        // the dice are too close, they need to bounce off of each other

        if (length == 0) {
            m_position.x += radius;
            other->m_position.x -= radius;
        }
        glm::vec3 norm = glm::normalize(m_position - other->m_position);
        if (length < radius) {
            // they are almost at the exact same spot, just choose a direction to bounce...
            // Using radius instead of 2*radius because we don't want to hit this condition
            // often since it makes the dice animation look jagged.  Give the else if condition a
            // chance to fix the problem instead.
            m_position = m_position + (radius-length) * norm;
            other->m_position = other->m_position - (radius-length) * norm;
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
    if (animationDone) {
        return false;
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
            glm::mat4 translate = glm::translate(m_position);
            m_model = translate * rotate * scale;
            return true;
        } else if (stoppedAnimationTime <= animationTime) {
            // all animations after the dice lands are done.
            animationDone = true;
            m_position.x = doneX;
            m_position.y = doneY;
            m_position.z = M_maxposz;
            glm::mat4 scale = glm::scale(glm::vec3(stoppedRadius, stoppedRadius, stoppedRadius));
            if (stoppedAngle != 0) {
                glm::quat q = glm::angleAxis(stoppedAngle, stoppedRotationAxis);
                qTotalRotated = glm::normalize(q * qTotalRotated);
            }
            checkQuaternion(qTotalRotated);
            glm::mat4 rotate = glm::toMat4(qTotalRotated);
            glm::mat4 translate = glm::translate(m_position);
            m_model = translate * rotate * scale;
            return true;
        } else if (stopped) {
            // we need to animate the move to the location it is supposed to go when it is done
            // rolling so that it is out of the way of rolling dice.
            animationTime += time;
            if (stoppedAnimationTime < animationTime) {
                animationTime = stoppedAnimationTime;
            }
            float r = radius - (radius - stoppedRadius) / stoppedAnimationTime * animationTime;
            m_position.x = (doneX - stoppedPositionX) / stoppedAnimationTime * animationTime +
                         stoppedPositionX;
            m_position.y = (doneY - stoppedPositionY) / stoppedAnimationTime * animationTime +
                         stoppedPositionY;
            m_position.z = (M_maxposz - stoppedPositionZ) / stoppedAnimationTime * animationTime +
                    stoppedPositionZ;
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
            glm::mat4 translate = glm::translate(m_position);
            m_model = translate * rotate * scale;
            return true;
        } else if (waitAfterDoneTime <= stoppedRotateTime) {
            // done settling the dice to the floor.  Wait for some time before moving the die to the
            // top of the screen.
            stopped = true;
            yAlign(upFace);
            glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
            glm::mat4 rotate = glm::toMat4(qTotalRotated);
            glm::mat4 translate = glm::translate(m_position);
            m_model = translate * rotate * scale;
            return true;
        } else if (goingToStopAnimationTime > stoppedRotateTime) {
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
            glm::mat4 translate = glm::translate(m_position);
            m_model = translate * rotate * scale;
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
            glm::mat4 translate = glm::translate(m_position);
            m_model = translate * rotate * scale;
            return true;
        }
    }

    // reset the position in the case that it got stuck outside the boundary
    if (m_position.x < -M_maxposx) {
        m_position.x = -M_maxposx;
    } else if (m_position.x > M_maxposx) {
        m_position.x = M_maxposx;
    }
    if (m_position.y < -M_maxposy) {
        m_position.y = -M_maxposy;
    } else if (m_position.y > M_maxposy) {
        m_position.y = M_maxposy;
    }
    if (m_position.z < -M_maxposz) {
        m_position.z = -M_maxposz;
    } else if (m_position.z > M_maxposz) {
        m_position.z = M_maxposz;
    }

    if (time > 0.5) {
        // the time that passed is too great, just not down the currentTime and move next time
        // through.
        glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
        checkQuaternion(qTotalRotated);
        glm::mat4 rotate = glm::toMat4(qTotalRotated);
        glm::mat4 translate = glm::translate(m_position);
        m_model = translate * rotate * scale;
        return false;
    }

    float speed = glm::length(velocity);

    if (speed != 0) {
        m_position += velocity*time;
        if (m_position.x > M_maxposx || m_position.x < -M_maxposx) {
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
        if (m_position.y > M_maxposy || m_position.y < -M_maxposy) {
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
        if (m_position.z > M_maxposz || m_position.z < -M_maxposz) {
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

    if (M_reverseGravity && (m_position.z > M_maxposz || M_maxposz - m_position.z < errorVal) && fabs(velocity.z) < errorVal) {
        goingToStop = true;
    } else if (!M_reverseGravity && (m_position.z < -M_maxposz || m_position.z + M_maxposz < errorVal) && fabs(velocity.z) < errorVal) {
        goingToStop = true;
    }

    if (goingToStop) {
        upFace = calculateUpFace();

        // return the actual upFace index as opposed to index into the symbol array.  This is
        // required because Java needs this information so that it can pass it back to us when
        // drawing stopped dice.  If we just used the index into the symbol array, we might get
        // a different colored die face than the original one.
        result = getUpFaceIndex(upFace);

        angularVelocity.setAngularSpeed(0);
        velocity = {0.0f, 0.0f, 0.0f};
        stoppedPositionX = m_position.x;
        stoppedPositionY = m_position.y;
        stoppedPositionZ = m_position.z;
    }

    if (angularVelocity.speed() != 0 && glm::length(angularVelocity.spinAxis()) > 0) {
        glm::quat q = glm::angleAxis(angularVelocity.speed()*time, angularVelocity.spinAxis());
        qTotalRotated = glm::normalize(q * qTotalRotated);
        float angularSpeed = angularVelocity.speed();
        angularSpeed -= viscosity * angularSpeed * time;
        angularVelocity.setAngularSpeed(angularSpeed);
    }

    bool needsRedraw = true;
    if (!goingToStop) {
        float difference = glm::length(m_position - prevPosition);
        if (difference < 0.01f) {
            needsRedraw = false;
        } else {
            prevPosition = m_position;
        }
    }

    glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
    checkQuaternion(qTotalRotated);
    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    glm::mat4 translate = glm::translate(m_position);
    m_model = translate * rotate * scale;

    return needsRedraw;
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

void DicePhysicsModel::positionDice(float x, float y, bool randomizeUpFace) {
    if (randomizeUpFace) {
        Random random;
        uint32_t upFace = random.getUInt(0, numberFaces - 1);
        positionDice(upFace, x, y);
    } else {
        uint32_t upFace = calculateUpFace();
        positionDice(upFace, x, y);
    }
}

void DicePhysicsModel::positionDice(uint32_t inUpFaceIndex, float x, float y) {
    // get rid of any previous rotation on the die.  When the dice are initialized, they are set
    // to have a random face up and a random rotation around the z axis (resetPositions() is called.
    // This is done so that if the dice are rolling, they will be fair.  We don't need that here.
    // They are being placed, not rolling.
    qTotalRotated = glm::quat{};

    uint32_t faceIndex = getFaceIndexForSymbol(inUpFaceIndex);
    glm::vec3 zaxis = glm::vec3(0.0, 0.0, 1.0);

    m_position.x = x;
    m_position.y = y;
    m_position.z = M_maxposz;
    stoppedPositionX = x;
    stoppedPositionY = y;

    velocity = { 0.0, 0.0, 0.0 };

    angularVelocity.setAngularSpeed(0.0);
    angularVelocity.setSpinAxis(glm::vec3(0.0,0.0,0.0));

    glm::vec3 normalVector = {};
    float angle = 0;

    // getAngleAxis uses the model matrix... initialize here temporarily and then reinitialize
    // once we get the rotation to do on the die.
    glm::mat4 scale = glm::scale(glm::vec3(stoppedRadius, stoppedRadius, stoppedRadius));
    glm::mat4 translate = glm::translate(m_position);
    m_model = translate * scale;
    getAngleAxis(faceIndex, angle, normalVector);

    glm::vec3 cross = glm::cross(normalVector, zaxis);
    // if the cross product is zero, the normalVector must be along the z-axis. rotate around the
    // y-axis.
    if (glm::length(cross) == 0.0f) {
        cross = {0.0f, 1.0f, 0.0f};
    }

    if (angle != 0) {
        glm::quat quaternian = glm::angleAxis(angle, glm::normalize(cross));
        checkQuaternion(quaternian);

        qTotalRotated = glm::normalize(quaternian);
        checkQuaternion(qTotalRotated);
    }

    glm::mat4 rotate = glm::toMat4(qTotalRotated);
    m_model = translate * rotate * scale;

    yAlign(faceIndex);
    if (stoppedAngle != 0) {
        glm::quat q = glm::angleAxis(stoppedAngle, stoppedRotationAxis);
        checkQuaternion(qTotalRotated);
        checkQuaternion(q);
        qTotalRotated = glm::normalize(q*qTotalRotated);
        stoppedAngle = 0;
    }

    rotate = glm::toMat4(qTotalRotated);
    m_model = translate * rotate * scale;
    stopped = true;
    animationDone = true;
    goingToStop = true;
    result = inUpFaceIndex;
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
    // if the cross product is zero, the normalVector must be along the z-axis. rotate along the
    // y-axis.
    if (glm::length(cross) == 0.0f) {
        cross = {0.0f, 1.0f, 0.0f};
    }

    if (angle != 0) {
        glm::quat quaternian = glm::angleAxis(angle, glm::normalize(cross));
        checkQuaternion(quaternian);

        glm::quat quaternian2 = glm::angleAxis(angle2, zaxis);
        checkQuaternion(quaternian2);

        qTotalRotated = glm::normalize(quaternian2 * quaternian);
        checkQuaternion(qTotalRotated);
    }

    m_position.x = random.getFloat(-M_maxposx, M_maxposx);
    m_position.y = random.getFloat(-M_maxposy, M_maxposy);
    if (M_reverseGravity) {
        m_position.z = -M_maxposz;
    } else {
        m_position.z = M_maxposz;
    }

    prevPosition = m_position;

    float factor = 10;
    velocity.x = random.getFloat(-factor, factor);
    velocity.y = random.getFloat(-factor, factor);
    velocity.x = random.getFloat(-factor, factor);
}

void DiceModelCube::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    Vertex vertex = {};
    vertex.mode = Vertex::MODE_EDGE_DISTANCE;

    uint32_t totalNbrImages = texAtlas->getNbrImages();
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

        TextureImage textureCoord = texAtlas->getTextureCoordinates(symbols[0]);
        switch (i) {
        case 0:
            vertex.texCoord = {textureCoord.left, textureCoord.bottom};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[2] + normals[5]);
            break;
        case 1:
            vertex.texCoord = {textureCoord.left, textureCoord.top};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[3] + normals[2]);
            break;
        case 2:
            vertex.texCoord = {textureCoord.right, textureCoord.top};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[4] + normals[3]);
            break;
        case 3:
        default:
            vertex.texCoord = {textureCoord.right, textureCoord.bottom};
            vertex.cornerNormal = glm::normalize(normals[0] + normals[5] + normals[4]);
            break;
        }
        vertex.color = color(i);
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
        textureCoord = texAtlas->getTextureCoordinates(symbols[1%symbols.size()]);
        switch (i) {
        case 0:
            vertex.texCoord = {textureCoord.left, textureCoord.top};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[2] + normals[5]);
            break;
        case 1:
            vertex.texCoord = {textureCoord.left, textureCoord.bottom};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[3] + normals[2]);
            break;
        case 2:
            vertex.texCoord = {textureCoord.right, textureCoord.bottom};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[4] + normals[3]);
            break;
        case 3:
        default:
            vertex.texCoord = {textureCoord.right, textureCoord.top};
            vertex.cornerNormal = glm::normalize(normals[1] + normals[5] + normals[4]);
            break;
        }
        vertex.color = color(i+3);
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
        vertex.color = color(i);
        TextureImage textureCoord = texAtlas->getTextureCoordinates(symbols[(i+2)%symbols.size()]);

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
        vertex.texCoord = {textureCoord.left, textureCoord.top};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[0] + normals[(i+3)%4+2]);
        vertices.push_back(vertex);

        cubeTop(vertex.pos, (i+1)%4);
        vertex.texCoord = {textureCoord.left, textureCoord.bottom};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[0] + normals[(i+1)%4+2]);
        vertices.push_back(vertex);

        cubeBottom(vertex.pos, i);
        vertex.texCoord = {textureCoord.right, textureCoord.top};
        vertex.cornerNormal = glm::normalize(normals[i+2] + normals[1] + normals[(i+3)%4+2]);
        vertices.push_back(vertex);

        cubeBottom(vertex.pos, (i+1)%4);
        vertex.texCoord = {textureCoord.right, textureCoord.bottom};
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
        glm::vec4 p04 = m_model * glm::vec4(vertices[2].pos, 1.0f);
        glm::vec4 q4 = m_model * glm::vec4(vertices[0].pos, 1.0f);
        glm::vec4 r4 = m_model * glm::vec4(vertices[6].pos, 1.0f);
        glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
        glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
        glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
        axis = glm::normalize(glm::cross(q - r, q - p0));
        angle = glm::acos(glm::dot(axis, zaxis));
    } else if (faceIndex == 1) {
        /* what was the bottom face at the start */
        glm::vec4 p04 = m_model * glm::vec4(vertices[7].pos, 1.0f);
        glm::vec4 q4 = m_model * glm::vec4(vertices[1].pos, 1.0f);
        glm::vec4 r4 = m_model * glm::vec4(vertices[3].pos, 1.0f);
        glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
        glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
        glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
        axis = glm::normalize(glm::cross(q - r, q - p0));
        angle = glm::acos(glm::dot(axis, zaxis));
    } else {
        /* what were the side faces at the start */
        glm::vec4 p04 = m_model * glm::vec4(vertices[4 * faceIndex].pos, 1.0f);
        glm::vec4 q4 = m_model * glm::vec4(vertices[1 + 4 * faceIndex].pos, 1.0f);
        glm::vec4 r4 = m_model * glm::vec4(vertices[3 + 4 * faceIndex].pos, 1.0f);
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
    zaxis = {0.0f, 0.0f, 1.0f};
    yaxis = {0.0f, 1.0f, 0.0f};
    glm::vec3 axis;
    if (faceIndex == 0 || faceIndex == 1) {
        glm::vec4 p24 = m_model * glm::vec4(vertices[2].pos, 1.0f);
        glm::vec4 p04 = m_model * glm::vec4(vertices[0].pos, 1.0f);
        glm::vec3 p2 = {p24.x, p24.y, p24.z};
        glm::vec3 p0 = {p04.x, p04.y, p04.z};
        axis = p2 - p0;
        if (faceIndex == 1) {
            axis = -axis;
        }
    } else {
        glm::vec4 p84 = m_model * glm::vec4(vertices[8+4*(faceIndex-2)].pos, 1.0f);
        glm::vec4 p94 = m_model * glm::vec4(vertices[9+4*(faceIndex-2)].pos, 1.0f);
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
        addVertices(texAtlas, p0, q, r, p0BottomCornerNormal, rNormal, qNormal, i * 2);

        // top
        topCorners(p0, q, r, i);
        addVertices(texAtlas, p0, q, r, p0TopCornerNormal, qNormal, rNormal, i * 2 + 1);
    }

    // indices - not really using these
    for (uint32_t i = 0; i < vertices.size(); i ++) {
        indices.push_back(i);
    }
}

float DiceModelHedron::p0ycoord(glm::vec3 const &q, glm::vec3 const &r) {
    float hypotenuseSquared = glm::dot(q-r, q-r);

    // if the y coord of p0 is less than 0.75f, then just use 0.75f.  hypotenuse squared can't be
    // less than 1 because that is the radius of the die (in the longest way in the x and z plane).
    if (hypotenuseSquared - 1 < 0.5625f) {
        return 0.75f;
    }

    float height = glm::sqrt(hypotenuseSquared - 1);

    return height;
}

void DiceModelHedron::topCorners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, int i) {
    q = {glm::cos(4.0f*((i+1)%(numberFaces/2))*pi/numberFaces), 0.0f, glm::sin(4*((i+1)%(numberFaces/2))*pi/numberFaces)};
    r = {glm::cos(4*i*pi/numberFaces), 0.0f, glm::sin(4*i*pi/numberFaces)};
    p0 = {0.0f, p0ycoord(q, r), 0.0f};
}

void DiceModelHedron::bottomCorners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, int i) {
    q = {glm::cos(4*i*pi/numberFaces), 0.0f, glm::sin(4*i*pi/numberFaces)};
    r = {glm::cos(4.0f*((i+1)%(numberFaces/2))*pi/numberFaces), 0.0f, glm::sin(4*((i+1)%(numberFaces/2))*pi/numberFaces)};
    p0 = {0.0f, -p0ycoord(q, r), 0.0f};
}

void DiceModelHedron::addVertices(std::shared_ptr<TextureAtlas> const &textAtlas,
                                  glm::vec3 const &p0, glm::vec3 const &q, glm::vec3 const &r,
                                  glm::vec3 const &p0Normal, glm::vec3 const &qNormal, glm::vec3 const &rNormal,
                                  uint32_t i) {
    TextureImage textureCoords = textAtlas->getTextureCoordinates(symbols[i%symbols.size()]);
    float symbolHeightTextureSpace = textureCoords.bottom - textureCoords.top;
    float symbolWidthTextureSpace = fabs(textureCoords.right - textureCoords.left);

    // ratio of width to height of the current symbol, actually a/b in docs.
    float a = (symbolWidthTextureSpace*textAtlas->getImageWidth()) /
            (symbolHeightTextureSpace*textAtlas->getImageHeight());

    bool rotate = false;
    if (1/a < rotateThreshold && glm::length(r-q)/glm::length(p0-r) < rotateThreshold) {
        rotate = rotated[i%symbols.size()] = true;
        a = (symbolHeightTextureSpace*textAtlas->getImageHeight())/(symbolWidthTextureSpace*textAtlas->getImageWidth());
    }

    float k = a * glm::length((r+q)/2.0f-p0) / glm::length(r-q);

    glm::vec3 p2 = (1.0f - 1.0f/(2.0f * k + 2.0f)) * r + (1.0f/(2.0f*k+2.0f)) * q;
    glm::vec3 p3 = 1.0f/(2.0f*k+2.0f) * r + (1.0f-1.0f/(2.0f*k+2.0f)) * q;
    glm::vec3 p1 = p2 - 1.0f/(1.0f+k) * ((r+q)/2.0f - p0);
    glm::vec3 p1prime = p3 - 1.0f/(1.0f+k) * ((r+q)/2.0f - p0);

    Vertex vertex = {};
    vertex.mode = Vertex::MODE_EDGE_DISTANCE;

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
    if (rotate) {
        vertex.texCoord = {textureCoords.left + glm::length(0.5f*(r+q) - p0)/glm::length(p1-p2) *
                           symbolWidthTextureSpace,
                           textureCoords.top + symbolHeightTextureSpace*0.5f};
    } else {
        vertex.texCoord = {textureCoords.left + symbolWidthTextureSpace * 0.5,
                           textureCoords.bottom -
                           glm::length(0.5f * (r + q) - p0) / glm::length(p1 - p2) *
                           symbolHeightTextureSpace};
    }
    vertex.color = color(i);
    vertices.push_back(vertex);

    // left point
    vertex.pos = q;
    vertex.cornerNormal = qNormal;
    if (rotate) {
        vertex.texCoord = {textureCoords.left,
                           textureCoords.top -
                           glm::length(q-p3)/glm::length(p2-p3) * symbolHeightTextureSpace};
    } else {
        vertex.texCoord = {textureCoords.left -
                           glm::length(q - p3) / glm::length(p2 - p3) * symbolWidthTextureSpace,
                           textureCoords.bottom};
    }
    vertex.color = color(i+2);
    vertices.push_back(vertex);

    // right point
    vertex.pos = r;
    vertex.cornerNormal = rNormal;
    if (rotate) {
        vertex.texCoord = {textureCoords.left,
                           textureCoords.bottom +
                           glm::length(r-p2)/glm::length(p2-p3) * symbolHeightTextureSpace};
    } else {
        vertex.texCoord = {textureCoords.right +
                           glm::length(r - p2) / glm::length(p2 - p3) * symbolWidthTextureSpace,
                           textureCoords.bottom};
    }
    vertex.color = color(i+2);
    vertices.push_back(vertex);
}

void DiceModelHedron::getAngleAxis(uint32_t face, float &angle, glm::vec3 &axis) {
    const uint32_t nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 zaxis = glm::vec3(0.0f,0.0f,1.0f);
    glm::vec4 p04 = m_model * glm::vec4(vertices[face * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 q4 = m_model * glm::vec4(vertices[1 + face * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 r4 = m_model * glm::vec4(vertices[2 + face * nbrVerticesPerFace].pos, 1.0f);
    glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
    glm::vec3 q = glm::vec3(q4.x, q4.y, q4.z);
    glm::vec3 r = glm::vec3(r4.x, r4.y, r4.z);
    axis = glm::normalize(glm::cross(q-r, q-p0));
    angle = glm::acos(glm::dot(axis, zaxis));
}

uint32_t DiceModelHedron::getUpFaceIndex(uint32_t i) {
    return i;
}

void DiceModelHedron::yAlign(uint32_t faceIndex) {
    // for dice where rotated is true, the die will be rotated in the opposite direction for OpenGL
    // than for Vulkan.  This is to make the texture right side up in OpenGL.  As to why it is
    // upside down if this is not done, I don't know.
    const uint32_t nbrVerticesPerFace = static_cast<uint32_t>(vertices.size()) / numberFaces;
    glm::vec3 yaxis;
    glm::vec3 zaxis;
    glm::vec3 xaxis;
    xaxis = {1.0f, 0.0f, 0.0f};
    zaxis = {0.0f, 0.0f, 1.0f};
    yaxis = {0.0f, 1.0f, 0.0f};
    glm::vec3 axis;

    glm::vec4 p04 = m_model * glm::vec4(vertices[0 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 q4 = m_model * glm::vec4(vertices[1 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 r4 = m_model * glm::vec4(vertices[2 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec3 p0 = {p04.x, p04.y, p04.z};
    glm::vec3 q = {q4.x, q4.y, q4.z};
    glm::vec3 r = {r4.x, r4.y, r4.z};
    axis = p0 - (0.5f * (r + q));

    float angle = 0.0f;
    if (rotated[getUpFaceIndex(faceIndex)%symbols.size()]) {
        angle = glm::acos(glm::dot(glm::normalize(axis), xaxis));
        if (axis.y > 0) {
            angle = -angle;
        }
    } else {
        angle = glm::acos(glm::dot(glm::normalize(axis), yaxis));
        if (axis.x < 0) {
            angle = -angle;
        }
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
    for (uint32_t i = 0; i < vertices.size(); i ++) {
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
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[2] + faceNormals[3] + faceNormals[16] + faceNormals[17] + faceNormals[18]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[14] + faceNormals[15] + faceNormals[16]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[0] + faceNormals[4] + faceNormals[12] + faceNormals[13] + faceNormals[14]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    // now for the bottom
    // the normal for p0.
    p0Normal = glm::normalize(faceNormals[5] + faceNormals[6] + faceNormals[7] + faceNormals[8] + faceNormals[9]);

    qNormal = glm::normalize(faceNormals[5] + faceNormals[9] + faceNormals[15] + faceNormals[16] + faceNormals[17]);
    rNormal = glm::normalize(faceNormals[5] + faceNormals[6] + faceNormals[17] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[6] + faceNormals[7] + faceNormals[10] + faceNormals[11] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[7] + faceNormals[8] + faceNormals[11] + faceNormals[12] + faceNormals[13]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[8] + faceNormals[9] + faceNormals[13] + faceNormals[14] + faceNormals[15]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = glm::normalize(faceNormals[5] + faceNormals[9] + faceNormals[15] + faceNormals[16] + faceNormals[17]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    // now the middle
    p0Normal = glm::normalize(faceNormals[6] + faceNormals[7] + faceNormals[10] + faceNormals[11] + faceNormals[19]);
    qNormal = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10] + faceNormals[18] + faceNormals[19]);
    rNormal = glm::normalize(faceNormals[0] + faceNormals[1] + faceNormals[10] + faceNormals[11] + faceNormals[12]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[7] + faceNormals[8] + faceNormals[11] + faceNormals[12] + faceNormals[13]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[0] + faceNormals[4] + faceNormals[12] + faceNormals[13] + faceNormals[14]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[8] + faceNormals[9] + faceNormals[13] + faceNormals[14] + faceNormals[15]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[3] + faceNormals[4] + faceNormals[14] + faceNormals[15] + faceNormals[16]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[5] + faceNormals[9] + faceNormals[15] + faceNormals[16] + faceNormals[17]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[2] + faceNormals[3] + faceNormals[16] + faceNormals[17] + faceNormals[18]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[5] + faceNormals[6] + faceNormals[17] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    rNormal = qNormal;
    qNormal = p0Normal;
    p0Normal = rNormal;
    rNormal = glm::normalize(faceNormals[1] + faceNormals[2] + faceNormals[10] + faceNormals[18] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);
    i++;

    qNormal = rNormal;
    rNormal = p0Normal;
    p0Normal = qNormal;
    qNormal = glm::normalize(faceNormals[6] + faceNormals[7] + faceNormals[10] + faceNormals[11] + faceNormals[19]);
    corners(p0,q,r,i);
    addVertices(texAtlas, p0, q, r, p0Normal, qNormal, rNormal, i);

    // indices - not really using these
    for (i = 0; i < vertices.size(); i ++) {
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

    TextureImage textureCoords = texAtlas->getTextureCoordinates(symbols[i % symbols.size()]);

    Vertex vertex = {};
    vertex.mode = Vertex::MODE_EDGE_DISTANCE;

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
    vertex.color = color(i);
    vertices.push_back(vertex);

    vertex.pos = b;
    vertex.cornerNormal = cornerNormalB;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    vertex.pos = e;
    vertex.cornerNormal = cornerNormalE;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    // right bottom triangle
    // not really using textures for this triangle
    vertex.pos = b;
    vertex.cornerNormal = cornerNormalB;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    vertex.pos = c;
    vertex.cornerNormal = cornerNormalC;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 2);
    vertices.push_back(vertex);

    vertex.pos = p1;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    // left bottom triangle
    // not really using textures for this triangle
    vertex.pos = e;
    vertex.cornerNormal = cornerNormalE;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    vertex.pos = d;
    vertex.cornerNormal = cornerNormalD;
    vertex.texCoord = {0.0f, 0.0f};
    vertex.color = color(i + 2);
    vertices.push_back(vertex);

    // bottom texture triangle
    vertex.pos = p1;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {textureCoords.left, textureCoords.top};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    vertex.pos = c;
    vertex.cornerNormal = cornerNormalC;
    vertex.texCoord = {textureCoords.left, textureCoords.bottom};
    vertex.color = color(i + 2);
    vertices.push_back(vertex);

    vertex.pos = d;
    vertex.cornerNormal = cornerNormalD;
    vertex.texCoord = {textureCoords.right, textureCoords.bottom};
    vertex.color = color(i + 2);
    vertices.push_back(vertex);

    // top texture triangle
    vertex.pos = p1;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {textureCoords.left, textureCoords.top};
    vertex.color = color(i + 1);
    vertices.push_back(vertex);

    vertex.pos = d;
    vertex.cornerNormal = cornerNormalD;
    vertex.texCoord = {textureCoords.right, textureCoords.bottom};
    vertex.color = color(i + 2);
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.cornerNormal = vertex.normal;
    vertex.texCoord = {textureCoords.right, textureCoords.top};
    vertex.color = color(i + 1);
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
    for (i = 0; i < vertices.size(); i ++) {
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

    glm::vec4 a4 = m_model * glm::vec4(vertices[faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 b4 = m_model * glm::vec4(vertices[1 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 c4 = m_model * glm::vec4(vertices[4 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
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
    zaxis = {0.0f, 0.0f, 1.0f};
    yaxis = {0.0f, 1.0f, 0.0f};
    glm::vec3 axis;

    glm::vec4 p14 = m_model * glm::vec4(vertices[5 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 c4 = m_model * glm::vec4(vertices[4 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
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

void DiceModelRhombicTriacontahedron::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    glm::vec3 p0, p1, p2, p3;
    uint32_t vertices[4];
    glm::vec3 cornerNormal[4];

    for (uint32_t faceIndex = 0; faceIndex < 30; faceIndex++) {
        corners(faceIndex, vertices[0], vertices[1], vertices[2], vertices[3]);
        for (uint32_t i = 0; i < 4; i++) {
            cornerNormal[i] = {0.0f, 0.0f, 0.0f};
            for (auto &&faceIndexForVertex : facesForVertex(vertices[i])) {
                corners(faceIndexForVertex, p0, p1, p2, p3);
                cornerNormal[i] += glm::cross(p3 - p0, p1 - p0);
            }
            cornerNormal[i] = glm::normalize(cornerNormal[i]);
        }

        addVertices(texAtlas, cornerNormal[0], cornerNormal[1], cornerNormal[2], cornerNormal[3],
                    faceIndex);
    }
}

void DiceModelRhombicTriacontahedron::addVertices(std::shared_ptr<TextureAtlas> const &textAtlas,
                                                  glm::vec3 const &cornerNormal0,
                                                  glm::vec3 const &cornerNormal1,
                                                  glm::vec3 const &cornerNormal2,
                                                  glm::vec3 const &cornerNormal3,
                                                  uint32_t faceIndex) {
    glm::vec3 p0{};
    glm::vec3 p1{};
    glm::vec3 p2{};
    glm::vec3 p3{};
    corners(faceIndex, p0, p1, p2, p3);

    TextureImage textureCoords = textAtlas->getTextureCoordinates(symbols[faceIndex%symbols.size()]);
    float symbolHeightTextureSpace = (textureCoords.bottom - textureCoords.top);
    float symbolWidthTextureSpace = fabs(textureCoords.right - textureCoords.left);

    // ratio of width to height actually a/b in docs.
    float a = symbolWidthTextureSpace * textAtlas->getImageWidth() /
            (0.5f*symbolHeightTextureSpace*textAtlas->getImageHeight());
    bool rotate = false;
    if ((symbolHeightTextureSpace*textAtlas->getImageHeight()) /
        (symbolWidthTextureSpace*textAtlas->getImageWidth()) < rotateThreshold) {
        rotate = rotated[faceIndex%symbols.size()] = true;
        a = (symbolHeightTextureSpace*textAtlas->getImageHeight()) /
                (0.5f*symbolWidthTextureSpace*textAtlas->getImageWidth());
    }

    float k = a * glm::length((p1+p3)/2.0f-p0) / glm::length(p1-p3);

    glm::vec3 v = (1.0f - 1.0f/(2.0f * k + 2.0f)) * p1 + (1.0f/(2.0f*k+2.0f)) * p3;
    glm::vec3 u = 1.0f/(2.0f*k+2.0f) * p1 + (1.0f-1.0f/(2.0f*k+2.0f)) * p3;
    glm::vec3 t1 = v - 1.0f/(1.0f+k) * ((p1+p3)/2.0f - p0);
    glm::vec3 t0 = u - 1.0f/(1.0f+k) * ((p1+p3)/2.0f - p0);
    glm::vec3 t2 = v - 1.0f/(1.0f+k) * ((p1+p3)/2.0f - p2);
    glm::vec3 t3 = u - 1.0f/(1.0f+k) * ((p1+p3)/2.0f - p2);

    Vertex vertex{};
    vertex.mode = Vertex::MODE_EDGE_DISTANCE;
    vertex.corner1 = p3;
    vertex.corner2 = p2;
    vertex.corner3 = p1;
    vertex.corner4 = p0;
    vertex.corner5 = {1000.0, 1000.0, 1000.0};
    vertex.normal = glm::normalize(glm::cross(p3-p0, p1-p0));

    vertex.pos = p0;
    vertex.color = color(faceIndex);
    vertex.cornerNormal = cornerNormal0;
    if (rotate) {
        vertex.texCoord = {textureCoords.left + (0.5*symbolWidthTextureSpace +
                           glm::length(0.5f * (p1 + p3) - p0) / glm::length(t1 - t2) * symbolWidthTextureSpace),
                           textureCoords.bottom - 0.5f * symbolHeightTextureSpace};
    } else {
        vertex.texCoord = {textureCoords.left + 0.5*symbolWidthTextureSpace,
                           textureCoords.bottom - 0.5f * symbolHeightTextureSpace -
                           glm::length(0.5f * (p1 + p3) - p0) / glm::length(t1 - t2) *
                           symbolHeightTextureSpace};
    }
    vertices.push_back(vertex);

    vertex.pos = p1;
    vertex.color = color(faceIndex+1);
    vertex.cornerNormal = cornerNormal1;
    if (rotate) {
        vertex.texCoord = {textureCoords.left + 0.5*symbolWidthTextureSpace,
                           textureCoords.bottom +
                           symbolHeightTextureSpace * glm::length(0.5f*(t1+t2) - p1)/glm::length(t0-t1)};
    } else {
        vertex.texCoord = {textureCoords.right +
                           symbolWidthTextureSpace * glm::length(p1 - v) / glm::length(v - u),
                           textureCoords.bottom - symbolHeightTextureSpace * 0.5f};
    }
    vertices.push_back(vertex);

    vertex.pos = p2;
    vertex.color = color(faceIndex+2);
    vertex.cornerNormal = cornerNormal2;
    if (rotate) {
        vertex.texCoord = {textureCoords.left -
                           symbolWidthTextureSpace*glm::length(0.5f*(t3+t2) - p2)/glm::length(t2-t1),
                           textureCoords.bottom - symbolHeightTextureSpace * 0.5f};
    } else {
        vertex.texCoord = {textureCoords.left + symbolWidthTextureSpace * 0.5f,
                           textureCoords.bottom - 0.5f * symbolHeightTextureSpace +
                           glm::length(0.5f * (p1 + p3) - p2) / glm::length(t1 - t2) *
                           symbolHeightTextureSpace};
    }
    vertices.push_back(vertex);

    vertex.pos = p3;
    vertex.color = color(faceIndex+3);
    vertex.cornerNormal = cornerNormal3;
    if (rotate) {
        vertex.texCoord = {textureCoords.left + symbolWidthTextureSpace * 0.5f,
                           textureCoords.top -
                           symbolHeightTextureSpace*glm::length(0.5f*(t3+t0) - p3)/glm::length(t0-t1)};
    } else {
        vertex.texCoord = {textureCoords.left -
                           symbolWidthTextureSpace * glm::length(p3 - u) / glm::length(v - u),
                           textureCoords.bottom - symbolHeightTextureSpace * 0.5f};
    }
    vertices.push_back(vertex);

    indices.push_back(0 + faceIndex*4);
    indices.push_back(3 + faceIndex*4);
    indices.push_back(1 + faceIndex*4);
    indices.push_back(1 + faceIndex*4);
    indices.push_back(3 + faceIndex*4);
    indices.push_back(2 + faceIndex*4);
}

std::vector<uint32_t> DiceModelRhombicTriacontahedron::facesForVertex(uint32_t vertexNumber) {
    switch (vertexNumber) {
        case 0:
            return std::vector<uint32_t>{0, 12, 5};
        case 1:
            return std::vector<uint32_t>{0, 19, 23};
        case 2:
            return std::vector<uint32_t>{12, 5, 1, 7, 28};
        case 3:
            return std::vector<uint32_t>{19, 23, 8, 22, 13};
        case 4:
            return std::vector<uint32_t>{16, 29, 7, 6, 9};
        case 5:
            return std::vector<uint32_t>{21, 2, 8, 11, 15};
        case 6:
            return std::vector<uint32_t>{26, 16, 29};
        case 7:
            return std::vector<uint32_t>{26, 21, 2};
        case 8:
            return std::vector<uint32_t>{0, 12, 3, 27, 23};
        case 9:
            return std::vector<uint32_t>{0, 5, 17, 10, 19};
        case 10:
            return std::vector<uint32_t>{27, 3, 14};
        case 11:
            return std::vector<uint32_t>{17, 10, 4};
        case 12:
            return std::vector<uint32_t>{24, 20, 14};
        case 13:
            return std::vector<uint32_t>{18, 25, 4};
        case 14:
            return std::vector<uint32_t>{26, 16, 2, 24, 20};
        case 15:
            return std::vector<uint32_t>{26, 29, 18, 25, 21};
        case 16:
            return std::vector<uint32_t>{3, 1, 20, 14, 9};
        case 17:
            return std::vector<uint32_t>{1, 7, 9};
        case 18:
            return std::vector<uint32_t>{7, 28, 6};
        case 19:
            return std::vector<uint32_t>{17, 18, 4, 28, 6};
        case 20:
            return std::vector<uint32_t>{10, 25, 4, 13, 15};
        case 21:
            return std::vector<uint32_t>{8, 13, 15};
        case 22:
            return std::vector<uint32_t>{8, 22, 11};
        case 23:
            return std::vector<uint32_t>{27, 24, 14, 22, 11};
        case 24:
            return std::vector<uint32_t>{12, 3, 1};
        case 25:
            return std::vector<uint32_t>{5, 17, 28};
        case 26:
            return std::vector<uint32_t>{19, 10, 13};
        case 27:
            return std::vector<uint32_t>{23, 27, 22};
        case 28:
            return std::vector<uint32_t>{16, 20, 9};
        case 29:
            return std::vector<uint32_t>{29, 18, 6};
        case 30:
            return std::vector<uint32_t>{25, 21, 15};
        case 31:
            return std::vector<uint32_t>{2, 24, 11};
        default:
            throw std::runtime_error("Invalid vertex number for rhombic triacontahedron");
    }
}

void DiceModelRhombicTriacontahedron::corners(uint32_t faceIndex,
                                              glm::vec3 &p0, glm::vec3 &p1,
                                              glm::vec3 &p2, glm::vec3 &p3) {
    uint32_t v0, v1, v2, v3;
    corners(faceIndex, v0, v1, v2, v3);
    p0 = vertex(v0);
    p1 = vertex(v1);
    p2 = vertex(v2);
    p3 = vertex(v3);
}

void DiceModelRhombicTriacontahedron::corners(uint32_t faceIndex,
                                              uint32_t &p0, uint32_t &p1,
                                              uint32_t &p2, uint32_t &p3) {

    switch (faceIndex) {
        case 0:
            p0 = 8;
            p1 = 0;
            p2 = 9;
            p3 = 1;
            return;
        case 1:
            p0 = 16;
            p1 = 17;
            p2 = 2;
            p3 = 24;
            return;
        case 2:
            p0 = 14;
            p1 = 31;
            p2 = 5;
            p3 = 7;
            return;
        case 3:
            p0 = 16;
            p1 = 24;
            p2 = 8;
            p3 = 10;
            return;
        case 4:
            p0 = 20;
            p1 = 11;
            p2 = 19;
            p3 = 13;
            return;
        case 5:
            p0 = 2;
            p1 = 25;
            p2 = 9;
            p3 = 0;
            return;
        case 6:
            p0 = 4;
            p1 = 29;
            p2 = 19;
            p3 = 18;
            return;
        case 7:
            p0 = 4;
            p1 = 18;
            p2 = 2;
            p3 = 17;
            return;
        case 8:
            p0 = 5;
            p1 = 22;
            p2 = 3;
            p3 = 21;
            return;
        case 9:
            p0 = 4;
            p1 = 17;
            p2 = 16;
            p3 = 28;
            return;
        case 10:
            p0 = 9;
            p1 = 11;
            p2 = 20;
            p3 = 26;
            return;
        case 11:
            p0 = 5;
            p1 = 31;
            p2 = 23;
            p3 = 22;
            return;
        case 12:
            p0 = 8;
            p1 = 24;
            p2 = 2;
            p3 = 0;
            return;
        case 13:
            p0 = 20;
            p1 = 21;
            p2 = 3;
            p3 = 26;
            return;
        case 14:
            p0 = 16;
            p1 = 10;
            p2 = 23;
            p3 = 12;
            return;
        case 15:
            p0 = 5;
            p1 = 21;
            p2 = 20;
            p3 = 30;
            return;
        case 16:
            p0 = 14;
            p1 = 6;
            p2 = 4;
            p3 = 28;
            return;
        case 17:
            p0 = 9;
            p1 = 25;
            p2 = 19;
            p3 = 11;
            return;
        case 18:
            p0 = 15;
            p1 = 13;
            p2 = 19;
            p3 = 29;
            return;
        case 19:
            p0 = 3;
            p1 = 1;
            p2 = 9;
            p3 = 26;
            return;
        case 20:
            p0 = 14;
            p1 = 28;
            p2 = 16;
            p3 = 12;
            return;
        case 21:
            p0 = 15;
            p1 = 7;
            p2 = 5;
            p3 = 30;
            return;
        case 22:
            p0 = 23;
            p1 = 27;
            p2 = 3;
            p3 = 22;
            return;
        case 23:
            p0 = 8;
            p1 = 1;
            p2 = 3;
            p3 = 27;
            return;
        case 24:
            p0 = 14;
            p1 = 12;
            p2 = 23;
            p3 = 31;
            return;
        case 25:
            p0 = 15;
            p1 = 30;
            p2 = 20;
            p3 = 13;
            return;
        case 26:
            p0 = 15;
            p1 = 6;
            p2 = 14;
            p3 = 7;
            return;
        case 27:
            p0 = 23;
            p1 = 10;
            p2 = 8;
            p3 = 27;
            return;
        case 28:
            p0 = 2;
            p1 = 18;
            p2 = 19;
            p3 = 25;
            return;
        case 29:
            p0 = 15;
            p1 = 29;
            p2 = 4;
            p3 = 6;
            return;
        default:
            throw std::runtime_error("Invalid face index for rhombic triacontahedron");
    }
}

glm::vec3 DiceModelRhombicTriacontahedron::vertex(uint32_t vertexIndex) {
    float phi = (1+sqrtf(5.0f))/2;
    float scaleFactor = 2.5f;
    float nbr1 = 1.0f/scaleFactor;
    float nbr2 = phi/scaleFactor;
    float nbr3 = (1.0f + phi)/scaleFactor;

    switch (vertexIndex) {
        case 0:
            return {0.0f, nbr1, nbr3};
        case 1:
            return {0.0f, -nbr1, nbr3};
        case 2:
            return {0.0f, nbr3, nbr2};
        case 3:
            return {0.0f, -nbr3, nbr2};
        case 4:
            return {0.0f, nbr3, -nbr2};
        case 5:
            return {0.0f, -nbr3, -nbr2};
        case 6:
            return {0.0f, nbr1, -nbr3};
        case 7:
            return {0.0f, -nbr1, -nbr3};
        case 8:
            return {nbr2, 0.0f, nbr3};
        case 9:
            return {-nbr2, 0.0f, nbr3};
        case 10:
            return {nbr3, 0.0f, nbr1};
        case 11:
            return {-nbr3, 0.0f, nbr1};
        case 12:
            return {nbr3, 0.0f, -nbr1};
        case 13:
            return {-nbr3, 0.0f, -nbr1};
        case 14:
            return {nbr2, 0.0f, -nbr3};
        case 15:
            return {-nbr2, 0.0f, -nbr3};
        case 16:
            return {nbr3, nbr2, 0.0f};
        case 17:
            return {nbr1, nbr3, 0.0f};
        case 18:
            return {-nbr1, nbr3, 0.0f};
        case 19:
            return {-nbr3, nbr2, 0.0f};
        case 20:
            return {-nbr3, -nbr2, 0.0f};
        case 21:
            return {-nbr1, -nbr3, 0.0f};
        case 22:
            return {nbr1, -nbr3, 0.0f};
        case 23:
            return {nbr3, -nbr2, 0.0f};
        case 24:
            return {nbr2, nbr2, nbr2};
        case 25:
            return {-nbr2, nbr2, nbr2};
        case 26:
            return {-nbr2, -nbr2, nbr2};
        case 27:
            return {nbr2, -nbr2, nbr2};
        case 28:
            return {nbr2, nbr2, -nbr2};
        case 29:
            return {-nbr2, nbr2, -nbr2};
        case 30:
            return {-nbr2, -nbr2, -nbr2};
        case 31:
            return {nbr2, -nbr2, -nbr2};
        default:
            throw std::runtime_error("Incorrect vertex number for rhombic triacontahedron");
    }
}

void DiceModelRhombicTriacontahedron::getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis) {
    uint32_t const nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 zaxis{0.0f, 0.0f, 1.0f};

    glm::vec4 p04 = m_model * glm::vec4(vertices[0 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 p14 = m_model * glm::vec4(vertices[1 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec4 p24 = m_model * glm::vec4(vertices[2 + faceIndex * nbrVerticesPerFace].pos, 1.0f);
    glm::vec3 p0 = glm::vec3(p04.x, p04.y, p04.z);
    glm::vec3 p1 = glm::vec3(p14.x, p14.y, p14.z);
    glm::vec3 p2 = glm::vec3(p24.x, p24.y, p24.z);
    axis = glm::normalize(glm::cross(p0-p1, p2-p1));
    angle = glm::acos(glm::dot(axis, zaxis));
}

void DiceModelRhombicTriacontahedron::yAlign(uint32_t faceIndex) {
    const uint32_t nbrVerticesPerFace = static_cast<uint32_t>(vertices.size())/numberFaces;
    glm::vec3 xaxis;
    glm::vec3 yaxis;
    glm::vec3 zaxis;
    xaxis = {1.0f, 0.0f, 0.0f};
    zaxis = {0.0f, 0.0f, 1.0f};
    yaxis = {0.0f, 1.0f, 0.0f};
    glm::vec3 axis;

    glm::vec4 p04 = m_model * glm::vec4(vertices[0 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec4 p24 = m_model * glm::vec4(vertices[2 + nbrVerticesPerFace * faceIndex].pos, 1.0f);
    glm::vec3 p0 = {p04.x, p04.y, p04.z};
    glm::vec3 p2 = {p24.x, p24.y, p24.z};
    axis = p0 - p2;

    float angle = 0.0f;
    if (rotated[getUpFaceIndex(faceIndex)%symbols.size()]) {
        angle = glm::acos(glm::dot(glm::normalize(axis), xaxis));
        if (axis.y > 0) {
            angle = -angle;
        }
    } else {
        angle = glm::acos(glm::dot(glm::normalize(axis), yaxis));
        if (axis.x < 0) {
            angle = -angle;
        }
    }
    stoppedRotationAxis = zaxis;
    stoppedAngle = angle;
}

glm::mat4 DiceModelRhombicTriacontahedron::alterPerspective(glm::mat4 proj) {
    // I don't know why, but we need to invert the z-axis if using OpenGL for this dice type only.
    if (isOpenGl) {
        proj[0][2] *= -1;
        proj[1][2] *= -1;
        proj[2][2] *= -1;
        proj[3][2] *= -1;
    }

    return proj;
}

constexpr float DiceModelCoin::radius;

void DiceModelCoin::addEdgeVertices() {
    Vertex vertex{};
    glm::vec3 centerTop{0.0f, 0.0f, thickness/2};
    glm::vec3 centerBottom{0.0f, 0.0f, -thickness/2};
    size_t start = vertices.size();

    // not using the edge detection for these vertices.  We just set the normal equal to the corner
    // normal and let the shader interpolate the normal smoothly over the entire surface
    vertex.mode = Vertex::MODE_EDGE_DISTANCE;
    vertex.corner1 = {1000.0, 1000.0, 1000.0};
    vertex.corner2 = {-1000.0, 0.0, 1000.0};
    vertex.corner3 = {-1000.0, -1000.0, -1000.0};
    vertex.corner4 = {1000.0, 1000.0, 1000.0};
    vertex.corner5 = {1000.0, 1000.0, 1000.0};
    vertex.texCoord = {0.0f, 0.0f}; // not using textures for these triangles.
    for (uint32_t i = 0; i < nbrPoints; i++) {
        vertex.color = color(i);

        vertex.pos = {radius * glm::cos(2 * pi * i / nbrPoints),
                      radius * glm::sin(2 * pi * i / nbrPoints),
                      thickness/2};
        glm::vec3 normal = glm::normalize(vertex.pos - centerTop);
        vertex.cornerNormal = glm::normalize(normal + glm::vec3{0.0f, 0.0f, 1.0f});
        vertex.normal = vertex.cornerNormal;
        vertices.push_back(vertex);

        vertex.pos.z *= -1;
        vertex.cornerNormal = glm::normalize(normal + glm::vec3{0.0f, 0.0f, -1.0f});
        vertex.normal = vertex.cornerNormal;
        vertices.push_back(vertex);
    }

    for (uint32_t i = 0; i < nbrPoints; i++) {
        // the indices
        indices.push_back(start+2*i);
        indices.push_back(start+2*i+1);
        indices.push_back(start+2*((i+1)%nbrPoints));

        indices.push_back(start+2*((i+1)%nbrPoints));
        indices.push_back(start+2*i+1);
        indices.push_back(start+2*((i+1)%nbrPoints)+1);
    }
}

void DiceModelCoin::addFaceVertices(std::shared_ptr<TextureAtlas> const &texAtlas) {
    Vertex vertex{};
    glm::vec3 centerTop{0.0f, 0.0f, thickness/2};
    glm::vec3 centerBottom{0.0f, 0.0f, -thickness/2};
    size_t start = vertices.size();
    vertex.mode = Vertex::MODE_CENTER_DISTANCE;
    vertex.corner3 = {0, 0, 0};
    vertex.corner4 = {0, 0, 0};
    vertex.corner5 = {0, 0, 0};

    vertex.normal = {0.0f, 0.0f, 1.0f};
    vertex.cornerNormal = {0.0f, 0.0f, 1.0f};
    vertex.pos = centerTop;
    vertex.color = color(0);
    vertex.corner1 = centerTop;
    vertex.corner2 = {radius, 0, thickness/2};
    vertices.push_back(vertex);

    vertex.normal = {0.0f, 0.0f, -1.0f};
    vertex.cornerNormal = {0.0f, 0.0f, -1.0f};
    vertex.pos = centerBottom;
    vertex.color = color(2);
    vertex.corner1 = centerBottom;
    vertex.corner2 = {radius, 0, -thickness/2};
    vertices.push_back(vertex);

    for (uint32_t i = 0; i < nbrPoints; i++) {
        vertex.color = color(1);
        vertex.pos = {radius * glm::cos(2 * pi * i / nbrPoints),
                      radius * glm::sin(2 * pi * i / nbrPoints),
                      thickness/2};
        vertex.normal = {0.0f, 0.0f, 1.0f};
        vertex.cornerNormal = glm::normalize(vertex.normal + glm::normalize(vertex.pos - centerTop));
        vertex.corner1 = centerTop;
        vertex.corner2 = {radius, 0, thickness/2};
        vertices.push_back(vertex);

        vertex.color = color(3);
        vertex.pos.z *= -1;
        vertex.normal = {0.0f, 0.0f, -1.0f};
        vertex.cornerNormal = glm::normalize(vertex.normal + glm::normalize(vertex.pos - centerBottom));
        vertex.corner1 = centerBottom;
        vertex.corner2 = {radius, 0, -thickness/2};
        vertices.push_back(vertex);
    }

    TextureImage textureCoord0 = texAtlas->getTextureCoordinates(symbols[0]);
    TextureImage textureCoord1 = texAtlas->getTextureCoordinates(symbols[1]);

    glm::vec3 o{vertices[start+2+3*nbrPoints/4].pos};
    float xFactor0 = (textureCoord0.right - textureCoord0.left)/glm::length(vertices[start+2+nbrPoints/4].pos - o);
    float xFactor1 = (textureCoord1.right - textureCoord1.left)/glm::length(vertices[start+2+nbrPoints/4].pos - o);
    float yFactor0 = (textureCoord0.bottom - textureCoord0.top)/glm::length(vertices[start+2+5*nbrPoints/4].pos - o);
    float yFactor1 = (textureCoord1.bottom - textureCoord1.top)/glm::length(vertices[start+2+5*nbrPoints/4].pos - o);
    glm::vec3 v{glm::normalize(vertices[start+2+nbrPoints/4].pos - o)};
    vertices[start].texCoord = {textureCoord0.left + glm::dot(vertices[start].pos - o, v)*xFactor0,
                            textureCoord0.top +
                            glm::length(glm::cross(vertices[start].pos - o, v))*yFactor0};
    vertices[start+1].texCoord = {textureCoord1.left + glm::dot(vertices[start].pos - o, v)*xFactor1,
                            textureCoord1.top +
                            glm::length(glm::cross(vertices[start].pos - o, v))*yFactor1};
    for (uint32_t i = 0; i < nbrPoints; i++) {
        // texture coordinates
        glm::vec3 pos{vertices[start+2+2*i].pos};
        vertices[start+2+2*i].texCoord = {textureCoord0.left + glm::dot(pos-o, v)*xFactor0,
              textureCoord0.top +
              glm::dot(glm::cross(pos-o, v), glm::vec3{0.0f,0.0f,1.0f})*yFactor0};

        // we just use all the vectors from the front face here since they are the same lengths
        // as ones taken from the back face.
        vertices[start+2+2*i+1].texCoord = {textureCoord1.right-glm::dot(pos-o, v)*xFactor1,
              textureCoord1.top +
              glm::dot(glm::cross(pos-o, v), glm::vec3{0.0f,0.0f,1.0f})*yFactor1};

        // indices
        indices.push_back(start);
        indices.push_back(start+2+(2*i)%(nbrPoints*2));
        indices.push_back(start+2+(2*(i+1))%(nbrPoints*2));

        indices.push_back(start+1);
        indices.push_back(start+2+(2*(i+1)+1)%(nbrPoints*2));
        indices.push_back(start+2+(2*i+1)%(nbrPoints*2));
    }
}

void DiceModelCoin::loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
    // if these functions' call order is changed, yAlign and getAngleAxis need to be changed too.
    addFaceVertices(texAtlas);
    addEdgeVertices();
}

void DiceModelCoin::getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis) {
    glm::vec3 zaxis{0.0f, 0.0f, 1.0f};

    glm::vec4 top4 = m_model * glm::vec4(vertices[0].pos, 1.0f);
    glm::vec4 bottom4 = m_model * glm::vec4(vertices[1].pos, 1.0f);
    glm::vec3 top = glm::vec3(top4.x, top4.y, top4.z);
    glm::vec3 bottom = glm::vec3(bottom4.x, bottom4.y, bottom4.z);
    if (faceIndex == 0) {
        axis = glm::normalize(top - bottom);
    } else {
        axis = glm::normalize(bottom - top);
    }
    angle = glm::acos(glm::dot(axis, zaxis));
}

void DiceModelCoin::yAlign(uint32_t faceIndex) {
    glm::vec3 yaxis;
    glm::vec3 zaxis;
    zaxis = {0.0f, 0.0f, 1.0f};
    yaxis = {0.0f, 1.0f, 0.0f};
    glm::vec3 axis;

    glm::vec4 center4 = m_model * glm::vec4(vertices[0].pos, 1.0f);
    glm::vec4 top4 = m_model * glm::vec4(vertices[2 + nbrPoints/2].pos, 1.0f);
    glm::vec3 center = {center4.x, center4.y, center4.z};
    glm::vec3 top = {top4.x, top4.y, top4.z};
    axis = top - center;

    float angle = glm::acos(glm::dot(glm::normalize(axis), yaxis));
    if (axis.x < 0) {
        angle = -angle;
    }
    stoppedRotationAxis = zaxis;
    stoppedAngle = angle;
}
