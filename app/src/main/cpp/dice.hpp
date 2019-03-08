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
#ifndef RAINBOWDICE_DICE_HPP
#define RAINBOWDICE_DICE_HPP

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
#include <cstdint>
//#include "vulkanWrapper.hpp"
#include "text.hpp"

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 cornerNormal;
    glm::vec3 corner1;
    glm::vec3 corner2;
    glm::vec3 corner3;
    glm::vec3 corner4;
    glm::vec3 corner5;
    float mode;
    static const float MODE_EDGE_DISTANCE;
    static const float MODE_CENTER_DISTANCE;

    bool operator==(const Vertex& other) const;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class AngularVelocity {
public:
    AngularVelocity(float inAngularSpeed, glm::vec3 const &inSpinAxis) {
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
    void setSpinAxis(glm::vec3 const &inSpinAxis) {
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
    glm::vec3 acceleration(glm::vec3 const &sensorInputs) {
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
protected:
    std::vector<std::string> symbols;
    bool isOpenGl;

    /* vertex, index, and texture data for drawing the model */
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

public:
    explicit DiceModel(std::vector<std::string> const &inSymbols, bool inIsOpenGl = false)
    {
        isOpenGl = inIsOpenGl;
        symbols = inSymbols;
    }

    bool operator==(DiceModel const &other) {
        return symbols == other.symbols;
    }

    bool operator>(DiceModel const &other) {
        return symbols > other.symbols;
    }

    bool operator<(DiceModel const &other) {
        return symbols < other.symbols;
    }

    uint32_t getNumberOfSymbols() {
        return static_cast<uint32_t> (symbols.size());
    }

    std::vector<std::string> const &getSymbols() {
        return symbols;
    }

    std::vector<uint32_t> const &getIndices() {
        return indices;
    }

    std::vector<Vertex> const &getVertices() {
        return vertices;
    }

    virtual ~DiceModel() = default;
};

class DicePhysicsModel : public DiceModel {
protected:
    glm::quat qTotalRotated;
    uint32_t numberFaces;

    // for performing various rotations after the die is considered to have stopped moving
    // first for rotating so that the up face is flat to the screen and second so for rotating
    // the number so that it is upright.
    float stoppedAngle;
    glm::vec3 stoppedRotationAxis;

    /* model matrix */
    glm::mat4 m_model;

    glm::vec4 color(int i) {
        glm::vec4 color;
        if (m_color.size() == 4) {
            color.r = m_color[0];
            color.g = m_color[1];
            color.b = m_color[2];
            color.a = m_color[3];
        } else {
            glm::vec3 const &colorRainbow = colors[i%colors.size()];
            color.r = colorRainbow.r;
            color.g = colorRainbow.g;
            color.b = colorRainbow.b;
            color.a = 1.0f;
        }

        return color;
    }
private:
    static const std::vector<glm::vec3> colors;

    static float const maxposx;
    static float const maxposy;
    static float const maxposz;
    static float const gravity;
    static float const viscosity;
    static float const errorVal;
    static float const radius;
    static float const angularSpeedScaleFactor;
    static float const stoppedAnimationTime;
    static float const goingToStopAnimationTime;
    static float const waitAfterDoneTime;

    /* for the accelerometer data.  These data are saved between dice creation and deletion. */
    static Filter filter;

    std::chrono::high_resolution_clock::time_point prevTime;

    /* rotation */
    AngularVelocity angularVelocity;

    /* position */
    glm::vec3 acceleration;
    glm::vec3 velocity;
    glm::vec3 m_position;
    glm::vec3 prevPosition;

    bool goingToStop;
    float stoppedRotateTime;
    uint32_t upFace;

    bool stopped;
    bool animationDone;
    float doneX;
    float doneY;
    float stoppedPositionX;
    float stoppedPositionY;
    float animationTime;
    uint32_t result;
    std::vector<float> m_color;
public:
    // radius of a die when it is done rolling (It is shrinked and moved out of the rolling space).
    static float const stoppedRadius;

    /* Set previous position to a bogus value to make sure the die is drawn first thing */
    DicePhysicsModel(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                     uint32_t inNumberFaces, bool inIsOpenGl = false)
        : DiceModel(inSymbols, inIsOpenGl), qTotalRotated(), numberFaces(inNumberFaces),
          prevTime(std::chrono::high_resolution_clock::now()),
          angularVelocity(0.0f, glm::vec3(0.0f,0.0f,0.0f)),
          velocity(0.0f,0.0f,0.0f), m_position(0.0f, 0.0f, 0.0f),
          prevPosition(10.0f, 0.0f, 0.0f), goingToStop(false), stoppedRotateTime(0.0f),
          stopped(false), animationDone(false), doneX(0.0f), doneY(0.0f), animationTime(0.0f),
          m_color(inColor)
    {
    }

    DicePhysicsModel(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                     std::vector<float> const &inColor, uint32_t inNumberFaces,
                     bool inIsOpenGl = false)
        : DiceModel(inSymbols, inIsOpenGl), qTotalRotated(), numberFaces(inNumberFaces),
          prevTime(std::chrono::high_resolution_clock::now()),
          angularVelocity(0.0f, glm::vec3(0.0f,0.0f,0.0f)),
          velocity(0.0f,0.0f,0.0f), m_position(inPosition),
          prevPosition(10.0f, 0.0f, 0.0f), goingToStop(false), stoppedRotateTime(0.0f),
          stopped(false), animationDone(false), doneX(0.0f), doneY(0.0f), animationTime(0.0f),
          m_color(inColor)
    {
    }

    virtual glm::mat4 alterPerspective(glm::mat4 proj) {
        return proj;
    }

    glm::mat4 model() {
        return m_model;
    }

    glm::vec3 position() {
        return m_position;
    }

    void updateAcceleration(float x, float y, float z);
    bool updateModelMatrix();
    void calculateBounce(DicePhysicsModel *other);
    void animateMove(float x, float y) {
        // if doneX and doneY are not 0, then we have completed a previous stopped animation
        // and are now moving the die because of a reroll.  So store the previous doneX and doneY
        // as the current stoppedPositionX and stoppedPositionY. Also, make the stoppedAngle 0, so
        // no additional rotation is done.
        if (doneX != 0.0f && doneY != 0.0f) {
            stoppedPositionX = doneX;
            stoppedPositionY = doneY;
            stoppedAngle = 0.0f;
        }

        // if the dice is to be moved, then set animationDone to false and animationTime to 0.
        // otherwise just return so no animation is done.
        if (doneX != x || doneY != y) {
            doneX = x;
            doneY = y;
            animationDone = false;
            animationTime = 0.0f;
        }
    }
    void positionDice(uint32_t inFaceIndex, float x, float y);
    bool isStopped() { return stopped; }
    bool isStoppedAnimationDone() { return animationDone; }
    bool isStoppedAnimationStarted() { return doneY != 0.0f; }
    uint32_t getResult() { return result; }
    void resetPosition(float screenWidth, float screenHeight);
    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) = 0;
    uint32_t calculateUpFace();
    void randomizeUpFace(float screenWidth, float screenHeight);
    virtual uint32_t getUpFaceIndex(uint32_t index) { return index; }
    virtual uint32_t getFaceIndexForSymbol(uint32_t symbolIndex) { return symbolIndex; }
    virtual uint32_t getFaceIndexForSymbol(std::string symbol) {
        uint32_t i=0;
        for (auto &&s : symbols) {
            if (symbol == s) {
                return i;
            }
            i++;
        }
        return 0;  // shouldn't be reached.
    }
    virtual void getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis) = 0;
    virtual void yAlign(uint32_t faceIndex) = 0;

    std::vector<float> const &dieColor() { return m_color; }
};

class DiceModelCube : public DicePhysicsModel {
private:
    void cubeTop(glm::vec3 &pos, uint32_t i);
    void cubeBottom(glm::vec3 &pos, uint32_t i);
public:
    DiceModelCube(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                  bool inIsOpenGl = false)
        : DicePhysicsModel(inSymbols, inColor, 6, inIsOpenGl)
    {
    }

    DiceModelCube(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                  std::vector<float> const &inColor, bool inIsOpenGl = false)
        : DicePhysicsModel(inSymbols, inPosition, inColor, 6, inIsOpenGl)
    {
    }

    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual void getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis);
    virtual void yAlign(uint32_t faceIndex);
};


// For the dice that look like octahedron but have more sides.  Other polyhedra that
// have triangular faces can inherit from this class and use as is for the most part.
// loadModel will have to be changed of course.
class DiceModelHedron : public DicePhysicsModel {
private:
    static float const rotateThreshold;
    std::vector<bool> rotated;
protected:
    void addVertices(std::shared_ptr<TextureAtlas> const &textAtlas,
                     glm::vec3 const &p0, glm::vec3 const &q, glm::vec3 const &r,
                     glm::vec3 const &p0Normal, glm::vec3 const &qNormal, glm::vec3 const &rNormal, uint32_t i);
    void bottomCorners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, int i);
    void topCorners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, int i);
public:
    DiceModelHedron(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                    uint32_t inNumberFaces = 0, bool inIsOpenGl = false)
        : DicePhysicsModel(inSymbols, inColor, inNumberFaces, inIsOpenGl)
    {
        if (inNumberFaces == 0) {
            numberFaces = inSymbols.size()%2==0?inSymbols.size():inSymbols.size()*2;
        }
        for (int i = 0; i < symbols.size(); i++) {
            rotated.push_back(false);
        }
    }

    DiceModelHedron(std::vector<std::string> const &inSymbols,
                    glm::vec3 &inPosition,
                    std::vector<float> const &inColor,
                    uint32_t inNumberFaces = 0,
                    bool inIsOpenGl = false)
        : DicePhysicsModel(inSymbols, inPosition, inColor, inNumberFaces, inIsOpenGl)
    {
        if (inNumberFaces == 0) {
            numberFaces = inSymbols.size()%2==0?inSymbols.size():inSymbols.size()*2;
        }
        for (int i = 0; i < symbols.size(); i++) {
            rotated.push_back(false);
        }
    }

    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual void getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis);
    virtual uint32_t getUpFaceIndex(uint32_t i);
    virtual void yAlign(uint32_t faceIndex);
};

class DiceModelTetrahedron : public DiceModelHedron {
    void corners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, uint32_t i);
public:
    DiceModelTetrahedron(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                         bool inIsOpenGl = false)
        : DiceModelHedron(inSymbols, inColor, 4, inIsOpenGl)
    {
    }

    DiceModelTetrahedron(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                         std::vector<float> const &inColor, bool inIsOpenGl = false)
        : DiceModelHedron(inSymbols, inPosition, inColor, 4, inIsOpenGl)
    {
    }

    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual uint32_t getFaceIndexForSymbol(std::string symbol) {
        return DicePhysicsModel::getFaceIndexForSymbol(symbol);
    }

    virtual uint32_t getUpFaceIndex(uint32_t index) { return index; }
};

class DiceModelIcosahedron : public DiceModelHedron {
    void corners(glm::vec3 &p0, glm::vec3 &q, glm::vec3 &r, uint32_t i);
public:
    DiceModelIcosahedron(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                         bool inIsOpenGl = false)
            : DiceModelHedron(inSymbols, inColor, 20, inIsOpenGl)
    {
    }

    DiceModelIcosahedron(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                         std::vector<float> const &inColor, bool inIsOpenGl = false)
            : DiceModelHedron(inSymbols, inPosition, inColor, 20, inIsOpenGl)
    {
    }

    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual uint32_t getUpFaceIndex(uint32_t index) { return index; }
    virtual uint32_t getFaceIndexForSymbol(std::string symbol) {
        return DicePhysicsModel::getFaceIndexForSymbol(symbol);
    }
};

class DiceModelDodecahedron : public DicePhysicsModel {
private:
    void addVertices(std::shared_ptr<TextureAtlas> const &texAtlas,
                     glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 e,
                     glm::vec3 cornerNormalA, glm::vec3 cornerNormalB, glm::vec3 cornerNormalC,
                     glm::vec3 cornerNormalD, glm::vec3 cornerNormalE, uint32_t i);
    void corners(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c, glm::vec3 &d, glm::vec3 &e, uint32_t i);
public:
    DiceModelDodecahedron(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                          bool inIsOpenGl = false)
    : DicePhysicsModel(inSymbols, inColor, 12, inIsOpenGl)
            {
            }

    DiceModelDodecahedron(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                          std::vector<float> const &inColor, bool inIsOpenGl = false)
    : DicePhysicsModel(inSymbols, inPosition, inColor, 12, inIsOpenGl)
    {
    }

    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual void getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis);
    virtual void yAlign(uint32_t faceIndex);
};

class DiceModelRhombicTriacontahedron : public DicePhysicsModel {
private:
    static float const rotateThreshold;
    std::vector<bool> rotated;

    void addVertices(std::shared_ptr<TextureAtlas> const &textAtlas,
                     glm::vec3 const &cornerNormal0, glm::vec3 const &cornerNormal1,
                     glm::vec3 const &cornerNormal2, glm::vec3 const &cornerNormal3,
                     uint32_t faceIndex);
    glm::vec3 vertex(uint32_t vertexIndex);

    // this version of corners returns the four vectors describing the vertices defining the face
    // given by faceIndex.
    void corners(uint32_t faceIndex, glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2, glm::vec3 &p3);

    // this version of corners returns the four indices describing the vertices defining the face
    // given by faceIndex.
    void corners(uint32_t faceIndex, uint32_t &p0, uint32_t &p1, uint32_t &p2, uint32_t &p3);

    // returns a vector of face indices that contain the given vertex number.
    std::vector<uint32_t> facesForVertex(uint32_t vertexNumber);
public:
    DiceModelRhombicTriacontahedron(std::vector<std::string> const &inSymbols,
                                    std::vector<float> const &inColor, bool inIsOpenGl = false)
            : DicePhysicsModel(inSymbols, inColor, 30, inIsOpenGl)
    {
        for (int i = 0; i < symbols.size(); i++) {
            rotated.push_back(false);
        }
    }

    DiceModelRhombicTriacontahedron(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                                    std::vector<float> const &inColor, bool inIsOpenGl = false)
            : DicePhysicsModel(inSymbols, inPosition, inColor, 30, inIsOpenGl)
    {
        for (int i = 0; i < symbols.size(); i++) {
            rotated.push_back(false);
        }
    }

    virtual glm::mat4 alterPerspective(glm::mat4 proj);
    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual void getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis);
    virtual void yAlign(uint32_t faceIndex);
};

class DiceModelCoin : public DicePhysicsModel {
private:
    static constexpr uint32_t nbrPoints = 32;  // needs to be divisible by 8.
    static constexpr float radius = 1.0f;
    static constexpr float thickness = 0.3f;
    void addEdgeVertices();
    void addFaceVertices(std::shared_ptr<TextureAtlas> const &texAtlas);
public:
    DiceModelCoin(std::vector<std::string> const &inSymbols, std::vector<float> const &inColor,
                  bool inIsOpenGl = false)
            : DicePhysicsModel(inSymbols, inColor, 2, inIsOpenGl)
    {
    }

    DiceModelCoin(std::vector<std::string> const &inSymbols, glm::vec3 &inPosition,
                  std::vector<float> const &inColor, bool inIsOpenGl = false)
            : DicePhysicsModel(inSymbols, inPosition, inColor, 2, inIsOpenGl)
    {
    }

    virtual void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas);
    virtual void getAngleAxis(uint32_t faceIndex, float &angle, glm::vec3 &axis);
    virtual void yAlign(uint32_t faceIndex);
};

std::shared_ptr<DicePhysicsModel> createDice(std::vector<std::string> const &symbols,
                                             std::vector<float> const &color, bool isOpenGL = false);

#endif /* RAINBOWDICE_DICE_HPP */
