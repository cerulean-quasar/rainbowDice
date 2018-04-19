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

#include <vector>
#include <string>
#include <array>
#include <chrono>
#include <vulkan/vulkan.h>

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
    static const float maxposx;
    static const float maxposy;
    static const float maxposz;
    static const float gravity;
    static const float viscosity;
    static const float errorVal;
    static const float radius;
    std::chrono::high_resolution_clock::time_point prevTime;

    /* rotation */
    float angularSpeed;
    glm::vec3 spinAxis;

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
        : DiceModel(inSymbols), prevTime(std::chrono::high_resolution_clock::now()), angularSpeed(0), stopped(false)
    {
    }

    DicePhysicsModel(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
        : DiceModel(inSymbols), prevTime(std::chrono::high_resolution_clock::now()), angularSpeed(0.0f),spinAxis(0.0f,0.0f,0.0f), velocity(0.0f,0.0f,0.0f), position(inPosition), stopped(false)
    {
    }

    void setView();
    void updatePerspectiveMatrix(int surfaceWidth, int surfaceHeight);
    void updateAngularVelocity(glm::vec3 &vToDrag);
    void updateVelocity(float totalX, float totalY);
    void updateAcceleration(float x, float y, float z);
    void updateModelMatrix();
    void calculateBounce(DicePhysicsModel *other);
    bool isStopped() { return stopped; }
    std::string getResult() { return result; }
    void resetPosition();
    virtual void loadModel(TextureAtlas &texAtlas) = 0;
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

    void loadModel(TextureAtlas &texAtlas);
    std::string calculateUpFace();
};


// For the dice that look like octahedron but have more sides and other polyhedra that
// have triangular faces.
class DiceModelHedron : public DicePhysicsModel {
protected:
    uint32_t sides;

    void addVertices(glm::vec3 p0, glm::vec3 q, glm::vec3 r, uint32_t i, TextureAtlas &texAtlas);
public:
    DiceModelHedron(std::vector<std::string> &inSymbols)
        : DicePhysicsModel(inSymbols)
    {
    }

    DiceModelHedron(std::vector<std::string> &inSymbols, glm::vec3 &inPosition)
        : DicePhysicsModel(inSymbols, inPosition)
    {
    }

    virtual void loadModel(TextureAtlas &texAtlas);
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

    void loadModel(TextureAtlas &texAtlas);
    int getUpFaceIndex(int i);
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

    void loadModel(TextureAtlas &texAtlas);
    int getUpFaceIndex(int i);
};
#endif
