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
#ifndef DICE_HPP
#define DICE_HPP
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <deque>
#include <string>
#include <array>
#include <chrono>
#include "vulkanWrapper.hpp"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    uint32_t textureToUse;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
    bool operator==(const Vertex& other) const;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class AngularVelocity {
public:
    AngularVelocity(float inAngularSpeed, glm::vec3 inSpinAxis) {
        _spinAxis = inSpinAxis;
        setAngularSpeed(inAngularSpeed);
    }
    float speed() {
        return _angularSpeed;
    }
    glm::vec3 spinAxis() {
        return _spinAxis;
    }
    void setAngularSpeed(float inAngularSpeed) {
        if (inAngularSpeed > maxAngularSpeed) {
            _angularSpeed = maxAngularSpeed;
        } else {
            _angularSpeed = inAngularSpeed;
        }
    }
    void setSpinAxis(glm::vec3 inSpinAxis) {
        _spinAxis = inSpinAxis;
    }
private:
    static float const maxAngularSpeed;
    float _angularSpeed;
    glm::vec3 _spinAxis;
};

/* for applying a high pass filter on the acceleration sample data */
class Filter {
private:
    std::chrono::high_resolution_clock::time_point highPassAccelerationPrevTime;
    static const unsigned long highPassAccelerationMaxSize;
    struct high_pass_samples {
        glm::vec3 output;
        glm::vec3 input;
        float dt;
    };
    std::deque<high_pass_samples> highPassAcceleration;

public:
    Filter()
        :highPassAccelerationPrevTime(std::chrono::high_resolution_clock::now()) {

    }
    glm::vec3 acceleration(glm::vec3 sensorInputs) {
        float RC = 3.0f;
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - highPassAccelerationPrevTime).count();

        highPassAccelerationPrevTime = currentTime;

        unsigned long size = highPassAcceleration.size();
        glm::vec3 acceleration(0.0f, 0.0f, 0.0f);

        if (size == 0) {
            high_pass_samples sample;
            acceleration = sample.output = sample.input = sensorInputs;
            sample.dt = dt;
            highPassAcceleration.push_back(sample);
        } else {
            glm::vec3 nextOut;
            for (unsigned long i=1; i < size; i++) {
                high_pass_samples &sample = highPassAcceleration[i];
                high_pass_samples &prev = highPassAcceleration[i-1];
                float alpha = RC/(RC+sample.dt);
                sample.output = alpha*(prev.output + sample.input - prev.input);

            }
            high_pass_samples sample;
            float alpha = RC/(RC+dt);
            high_pass_samples &prev = highPassAcceleration.back();
            sample.output = alpha*(prev.output + sensorInputs - prev.input);
            sample.input = sensorInputs;
            sample.dt = dt;
            highPassAcceleration.push_back(sample);
            if (size + 1 > highPassAccelerationMaxSize) {
                highPassAcceleration.pop_front();
            }

            if (size < 100) {
                acceleration = sample.output;
            } else {
                acceleration = 20.0f * sample.output + sensorInputs - sample.output;
            }
        }
        return acceleration;
    }
};

class DiceModel {
public:
    std::vector<std::string> symbols;

    /* vertex, index, and texture data for drawing the model */
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    DiceModel(std::vector<std::string> &inSymbols)
    {
        symbols = inSymbols;
    }

    bool operator==(DiceModel other) {
        return symbols == other.symbols;
    }

    bool operator>(DiceModel other) {
        return symbols > other.symbols;
    }

    bool operator<(DiceModel other) {
        return symbols < other.symbols;
    }

    virtual ~DiceModel() {
        /* do nothing */
    }
};

class DicePhysicsModel : public DiceModel {
protected:
    static const std::vector<glm::vec3> colors;
    glm::quat qTotalRotated;
private:
    static float const maxposx;
    static float const maxposy;
    static float const maxposz;
    static float const gravity;
    static float const viscosity;
    static float const errorVal;
    static float const radius;
    static float const angularSpeedScaleFactor;

    /* for the accelerometer data.  These data are saved between dice creation and deletion. */
    static Filter filter;

    std::chrono::high_resolution_clock::time_point prevTime;

    /* rotation */
    AngularVelocity angularVelocity;

    /* position */
    glm::vec3 acceleration;
    glm::vec3 velocity;
    glm::vec3 position;

    bool stopped;
    std::string result;

public:
    /* MVP matrix */
    UniformBufferObject ubo = {};

    DicePhysicsModel(std::vector<std::string> &inSymbols)
        : DiceModel(inSymbols), qTotalRotated(), prevTime(std::chrono::high_resolution_clock::now()),
          angularVelocity(0.0f, glm::vec3(0.0f,0.0f,0.0f)),
          velocity(0.0f,0.0f,0.0f), position(0.0f, 0.0f, 0.0f), stopped(false)
    {
    }

    DicePhysicsModel(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
        : DiceModel(inSymbols), qTotalRotated(), prevTime(std::chrono::high_resolution_clock::now()),
          angularVelocity(0.0f, glm::vec3(0.0f,0.0f,0.0f)),
          velocity(0.0f,0.0f,0.0f), position(inPosition), stopped(false)
    {
    }

    void setView();
    void updatePerspectiveMatrix(int surfaceWidth, int surfaceHeight);
    void updateAcceleration(float x, float y, float z);
    void updateModelMatrix();
    void calculateBounce(DicePhysicsModel *other);
    bool isStopped() { return stopped; }
    std::string getResult() { return result; }
    void resetPosition();
    virtual void loadModel() = 0;
    virtual std::string calculateUpFace() = 0;
};

class DiceModelCube : public DicePhysicsModel {
private:
    void cubeTop(Vertex &vertex, uint32_t i);
    void cubeBottom(Vertex &vertex, uint32_t i);
public:
    DiceModelCube(std::vector<std::string> &inSymbols)
        : DicePhysicsModel(inSymbols)
    {
    }

    DiceModelCube(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
        : DicePhysicsModel(inSymbols, inPosition)
    {
    }

    virtual void loadModel();
    virtual std::string calculateUpFace();
};


// For the dice that look like octahedron but have more sides.  Other polyhedra that
// have triangular faces can inherit from this class and use as is for the most part.
// loadModel will have to be changed of course.
class DiceModelHedron : public DicePhysicsModel {
protected:
    uint32_t sides;

    void addVertices(glm::vec3 p0, glm::vec3 q, glm::vec3 r, uint32_t i);
public:
    DiceModelHedron(std::vector<std::string> &inSymbols)
        : DicePhysicsModel(inSymbols)
    {
    }

    DiceModelHedron(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
        : DicePhysicsModel(inSymbols, inPosition)
    {
    }

    virtual void loadModel();
    virtual std::string calculateUpFace();
    virtual int getUpFaceIndex(int i);
};

class DiceModelTetrahedron : public DiceModelHedron {
public:
    DiceModelTetrahedron(std::vector<std::string> &inSymbols)
        : DiceModelHedron(inSymbols)
    {
    }

    DiceModelTetrahedron(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
        : DiceModelHedron(inSymbols, inPosition)
    {
    }

    virtual void loadModel();
    virtual int getUpFaceIndex(int i);
};

class DiceModelIcosahedron : public DiceModelHedron {
public:
    DiceModelIcosahedron(std::vector<std::string> &inSymbols)
            : DiceModelHedron(inSymbols)
    {
    }

    DiceModelIcosahedron(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
            : DiceModelHedron(inSymbols, inPosition)
    {
    }

    virtual void loadModel();
    virtual int getUpFaceIndex(int i);
};

class DiceModelDodecahedron : public DicePhysicsModel {
private:
    static uint32_t const sides;

    void addVertices(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 e, uint32_t i);

public:
    DiceModelDodecahedron(std::vector<std::string> &inSymbols)
    : DicePhysicsModel(inSymbols)
            {
            }

    DiceModelDodecahedron(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
    : DicePhysicsModel(inSymbols, inPosition)
    {
    }

    virtual void loadModel();
    virtual std::string calculateUpFace();
};
#endif
