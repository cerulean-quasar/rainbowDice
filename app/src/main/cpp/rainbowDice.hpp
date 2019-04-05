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
#ifndef RAINBOWDICE_HPP
#define RAINBOWDICE_HPP
#include <list>
#include <set>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "rainbowDiceGlobal.hpp"
#include "diceDescription.hpp"
#include "dice.hpp"
#include "text.hpp"

/* for applying a high pass filter on the acceleration sample data */
class Filter {
private:
    std::chrono::high_resolution_clock::time_point highPassAccelerationPrevTime;
    static constexpr unsigned long highPassAccelerationMaxSize = 512;
    struct high_pass_samples {
        glm::vec3 output;
        glm::vec3 input;
        float dt;
    };
    std::deque<high_pass_samples> highPassAcceleration;

public:
    Filter()
            : highPassAccelerationPrevTime{std::chrono::high_resolution_clock::now()}
    {
    }

    glm::vec3 acceleration(glm::vec3 const &sensorInputs) {
        float RC = 3.0f;
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - highPassAccelerationPrevTime).count();

        highPassAccelerationPrevTime = currentTime;

        unsigned long size = highPassAcceleration.size();
        glm::vec3 acceleration(0.0f, 0.0f, 0.0f);

        if (dt > 1) {
            // the time diff is too long.  This is not good data. Just return the sensor inputs and
            // save the time difference for next call through.
            acceleration = sensorInputs;
        } else if (size == 0) {
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

template <typename GraphicsType>
class DiceGraphics {
public:
    inline typename GraphicsType::Buffer const &indexBuffer() { return m_indexBuffer; }
    inline typename GraphicsType::Buffer const &vertexBuffer() { return m_vertexBuffer; }

    bool needsReroll() {
        if (!m_die->isStopped()) {
            // should not happen!
            return false;
        }

        uint32_t result = m_die->getResult();

        for (auto const & rerollIndex : m_rerollIndices) {
            if (result % m_die->getNumberOfSymbols() == rerollIndex) {
                return true;
            }
        }

        return false;
    }

    inline std::shared_ptr<DicePhysicsModel> const &die() { return m_die; }
    inline std::vector<uint32_t> const &rerollIndices() { return m_rerollIndices; }
    inline bool isSelected() { return m_isSelected; }
    inline size_t nbrIndices() { return m_die->getIndices().size(); }
    inline bool isGL() { return false; }

    DiceGraphics(std::vector<std::string> const &symbols, std::vector<uint32_t> inRerollIndices,
                 std::vector<float> const &color,
                 std::shared_ptr<TextureAtlas> const &textureAtlas)
            : m_die{std::move(createDice(symbols, color, isGL()))},
              m_rerollIndices{std::move(inRerollIndices)},
              m_isSelected{false},
              m_vertexBuffer{},
              m_indexBuffer{}

    {
        m_die->loadModel(textureAtlas);
        //m_die->resetPosition();
    }

    virtual ~DiceGraphics() = default;
protected:
    std::shared_ptr<DicePhysicsModel> m_die;
    std::vector<uint32_t> m_rerollIndices;
    bool m_isSelected;

    /* vertex buffer and index buffer. the index buffer indicates which vertices to draw and in
     * the specified order.  Note, vertices can be listed twice if they should be part of more
     * than one triangle.
     */
    typename GraphicsType::Buffer m_vertexBuffer;
    typename GraphicsType::Buffer m_indexBuffer;
};

class RainbowDice {
public:
    enum AccelerationEventType {
        LINEAR_ACCELERATION_EVENT,
        GRAVITY_EVENT,
        ACCELEROMETER
    };

    virtual void initModels()=0;

    virtual void initThread()=0;

    virtual GraphicsDescription graphicsDescription()=0;

    virtual void drawFrame()=0;

    virtual bool updateUniformBuffer()=0;

    virtual bool allStopped()=0;

    virtual std::vector<std::vector<uint32_t>> getDiceResults()=0;

    virtual bool needsReroll()=0;

    virtual void animateMoveStoppedDice()=0;

    virtual void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &rerollSymbol,
                            std::vector<float> const &color)=0;

    virtual void recreateSwapChain(uint32_t width, uint32_t height)=0;

    virtual void resetPositions()=0;

    virtual void resetToStoppedPositions(std::vector<std::vector<uint32_t>> const &inUpFaceIndices)=0;

    virtual void addRollingDice() = 0;

    virtual void cleanupThread() = 0;

    virtual void setTexture(std::shared_ptr<TextureAtlas> texture) = 0;

    // returns true if it needs a redraw
    virtual bool tapDice(float x, float y) = 0;

    // returns true if any dice are selected, false otherwise.
    virtual bool diceSelected() = 0;

    virtual bool changeDice(std::string const &inDiceName,
                    std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions,
                    std::shared_ptr<TextureAtlas> inTexture) = 0;

    virtual bool rerollSelected() = 0;
    virtual bool addRerollSelected() = 0;

    // returns true if dice were deleted
    virtual bool deleteSelected() = 0;

    virtual void scale(float scaleFactor) {
        m_viewPoint.z /= scaleFactor;

        if (m_viewPoint.z > m_maxZ) {
            m_viewPoint.z = m_maxZ;
        } else if (m_viewPoint.z < m_minZ) {
            m_viewPoint.z = m_minZ;
        }

        setView();
    }

    virtual void scroll(float distanceX, float distanceY) = 0;

    void setView() {
        /* glm::lookAt takes the eye position, the center position, and the up axis as parameters */
        m_view = glm::lookAt(m_viewPoint, m_viewPointCenterPosition, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    virtual void updatePerspectiveMatrix(uint32_t surfaceWidth, uint32_t surfaceHeight) {
        /* perspective matrix: takes the perspective projection, the aspect ratio, near and far
         * view planes.
         */
        m_proj = glm::perspective(glm::radians(67.0f), surfaceWidth / (float) surfaceHeight,
                                  0.1f, 10.0f);
        getScreenCoordinatesInWorldSpace();
        DicePhysicsModel::setMaxXYZ(m_screenWidth/2.0f, m_screenHeight/2.0f, 1.0f);
    }

    virtual void resetView() {
        m_viewPoint = startViewPoint();
        m_viewPointCenterPosition = startViewPointCenterPosition();
        setView();
    }

    virtual void setDice(std::string const &inDiceName,
            std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescription,
            bool inIsModified) {
        m_diceName = inDiceName;
        m_diceDescriptions = inDiceDescription;
        m_isModifiedRoll = inIsModified;
    }

    void updateAcceleration(AccelerationEventType type, float x, float y, float z) {
        switch (type) {
            case AccelerationEventType::LINEAR_ACCELERATION_EVENT:
                m_linearAcceleration = 20.0f * glm::vec3{x, y, z};
                DicePhysicsModel::updateAcceleration(m_gravity+m_linearAcceleration);
                return;
            case AccelerationEventType::GRAVITY_EVENT:
                m_gravity = {x, y, z};
                DicePhysicsModel::updateAcceleration(m_gravity+m_linearAcceleration);
                return;
            case AccelerationEventType::ACCELEROMETER:
                DicePhysicsModel::updateAcceleration(m_filter.acceleration(glm::vec3{x, y, z}));
                return;
        }
    }

    std::vector<std::shared_ptr<DiceDescription>> const &getDiceDescriptions() {
        return m_diceDescriptions;
    }

    std::string diceName() {
        return m_diceName;
    }

    bool isModifiedRoll() { return m_isModifiedRoll; }

    RainbowDice()
            : m_screenWidth{2.0f},
              m_screenHeight{2.0f},
              m_viewPoint{startViewPoint()},
              m_viewPointCenterPosition{startViewPointCenterPosition()},
              m_isModifiedRoll{false},
              m_linearAcceleration{},
              m_gravity{0.0f, 0.0f, 9.8f},
              m_filter{}
    {}

    virtual ~RainbowDice() = default;

protected:
    float m_screenWidth;
    float m_screenHeight;

    std::string m_diceName;
    std::vector<std::shared_ptr<DiceDescription>> m_diceDescriptions;

    glm::mat4 m_proj;
    glm::mat4 m_view;

    glm::vec3 m_viewPoint;
    glm::vec3 m_viewPointCenterPosition;

    bool m_isModifiedRoll;

    static constexpr float m_maxZ = 10.0f;
    static constexpr float m_minZ = 1.5f;
    static constexpr float m_maxScroll = 10.0f;
    static glm::vec3 startViewPointCenterPosition() { return {0.0f, 0.0f, 0.0f}; }
    static glm::vec3 startViewPoint() { return {0.0f, 0.0f, 3.0f}; };

    void getScreenCoordinatesInWorldSpace() {
        glm::mat4 view = glm::lookAt(startViewPoint(), startViewPointCenterPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec4 worldZ{0.0f, 0.0f, 1.0f+DicePhysicsModel::stoppedRadius, 1.0f};
        glm::vec4 z = m_proj * view * worldZ;

        glm::vec4 plus = glm::vec4{z.w, z.w, z.z, z.w};
        glm::vec4 worldPlus = glm::inverse(view) * glm::inverse(m_proj) * plus;

        m_screenWidth = worldPlus.x/worldPlus.w * 2;
        m_screenHeight = worldPlus.y/worldPlus.w * 2;
    }

    void scroll(float distanceX, float distanceY, uint32_t windowWidth, uint32_t windowHeight) {
        float x = distanceX/windowWidth * 2.0f;
        float y = distanceY/windowHeight * 2.0f;

        m_viewPointCenterPosition.x += x;
        m_viewPointCenterPosition.y -= y;

        m_viewPoint.x += x;
        m_viewPoint.y -= y;

        if (m_viewPoint.x > m_maxScroll) {
            m_viewPoint.x = m_maxScroll;
            m_viewPointCenterPosition.x = m_maxScroll;
        } else if (m_viewPoint.x < -m_maxScroll) {
            m_viewPoint.x = -m_maxScroll;
            m_viewPointCenterPosition.x = -m_maxScroll;
        }

        if (m_viewPoint.y > m_maxScroll) {
            m_viewPoint.y = m_maxScroll;
            m_viewPointCenterPosition.y = m_maxScroll;
        } else if (m_viewPoint.y < -m_maxScroll) {
            m_viewPoint.y = -m_maxScroll;
            m_viewPointCenterPosition.y = -m_maxScroll;
        }

        setView();
    }

private:
    glm::vec3 m_linearAcceleration;
    glm::vec3 m_gravity;

    /* for the accelerometer data.  These data are saved between dice creation and deletion. */
    Filter m_filter;
};

template <typename DiceType>
class RainbowDiceGraphics : public RainbowDice {
public:
    void addRollingDice() override;
    bool tapDice(float x, float y, uint32_t width, uint32_t height);
    bool updateUniformBuffer() override;
    std::vector<std::vector<uint32_t >> getDiceResults() override;
    void resetToStoppedPositions(std::vector<std::vector<uint32_t>> const &upFaceIndices) override;
    void setDice(std::string const &inDiceName,
            std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions,
            bool inIsModifiedRoll) override;
    bool changeDice(std::string const &inDiceName,
                    std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions,
                    std::shared_ptr<TextureAtlas> inTexture) override;
    bool rerollSelected() override;
    bool addRerollSelected() override;
    bool deleteSelected() override;
    void animateMoveStoppedDice() override;

    bool diceSelected() override {
        for (auto const &dice : m_dice) {
            for (auto const &die : dice) {
                if (die->isSelected()) {
                    return true;
                }
            }
        }

        return false;
    }
    void loadObject(std::vector<std::string> const &symbols,
                                       std::vector<uint32_t> const &rerollIndices,
                                       std::vector<float> const &color) override {
        std::list<std::shared_ptr<DiceType>> dice;
        dice.push_back(createDie(symbols, rerollIndices, color));
        m_dice.push_back(std::move(dice));
    }

    void resetPositions() override {
        for (auto const &dice: m_dice) {
            for (auto const &die : dice) {
                die->die()->resetPosition();
            }
        }
    }

    bool needsReroll() override {
        for (auto const &dice : m_dice) {
            // only check the last dice in the list to see if it needs to be rerolled.  The other dice
            // in the list already got rerolled.  That is why there is a die after it.
            if (dice.empty()) {
                continue;
            }
            auto const &die = *(dice.rbegin());
            if (die->needsReroll()) {
                return true;
            }
        }

        return false;
    }

    bool allStopped() override {
        for (auto const &dice : m_dice) {
            for (auto const &die : dice) {
                if (!die->die()->isStopped() || !die->die()->isStoppedAnimationDone()) {
                    return false;
                }
            }
        }
        return true;
    }

    explicit RainbowDiceGraphics(bool inDrawRollingDice)
      : RainbowDice{},
        m_drawRollingDice{inDrawRollingDice},
        m_dice{}
    {
    }

    ~RainbowDiceGraphics() override = default;
protected:
    bool m_drawRollingDice;
    using DiceList = std::list<std::list<std::shared_ptr<DiceType>>>;
    DiceList m_dice;

    virtual std::shared_ptr<DiceType> createDie(std::vector<std::string> const &symbols,
                                                std::vector<uint32_t> const &inRerollIndices,
                                                std::vector<float> const &color) = 0;
    virtual std::shared_ptr<DiceType> createDie(std::shared_ptr<DiceType> const &inDice) = 0;
    virtual bool invertY() = 0;
    void moveDiceToStoppedPositions();
private:
    void addRerollDice(bool resetPosition);
    void moveDiceToStoppedRandomUpface();
};

// Template class functions

// returns true if we already have a result.
template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::changeDice(std::string const &inDiceName,
                std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions,
                std::shared_ptr<TextureAtlas> inTexture) {
    setTexture(std::move(inTexture));
    setDice(inDiceName, inDiceDescriptions, false);
    initModels();
    if (m_drawRollingDice) {
        // display rolling dice
        resetPositions();
        updateUniformBuffer();
        return false;
    } else {
        // Do not animate the roll, just display the result.
        resetPositions();
        moveDiceToStoppedPositions();
        while (needsReroll()) {
            addRerollDice(true);
            moveDiceToStoppedPositions();
        }
        return true;
    }
}

template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::tapDice(float x, float y, uint32_t width, uint32_t height) {
    float swidth = 2.0;
    float sheight = 2.0;
    float xnorm = x/width * swidth - swidth / 2.0f;
    float ynorm = invertY() ? -y/height * sheight + sheight / 2.0f
                            : y/height * sheight - sheight / 2.0f;
    glm::vec4 point1{xnorm, ynorm, 1.0f, 1.0f};
    glm::vec4 point2{xnorm, ynorm, -1.0f, 1.0f};

    glm::vec4 transformedP1 = glm::inverse(m_view) * glm::inverse(m_proj) * point1;
    glm::vec4 transformedP2 = glm::inverse(m_view) * glm::inverse(m_proj) * point2;

    glm::vec3 start = glm::vec3{transformedP1.x, transformedP1.y, transformedP1.z}/transformedP1.w;
    glm::vec3 line =  glm::vec3{transformedP2.x, transformedP2.y, transformedP2.z}/transformedP2.w - start;

    line = glm::normalize(line);
    bool needsRedraw = false;
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            float distanceToLine = glm::length(glm::cross(line, die->die()->position() - start));
            if (distanceToLine < DicePhysicsModel::stoppedRadius) {
                die->toggleSelected();
                needsRedraw = true;
                break;
            }
        }
        if (needsRedraw) {
            break;
        }
    }

    return needsRedraw;
}

template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::updateUniformBuffer() {
    for (auto iti = m_dice.begin(); iti != m_dice.end(); iti++) {
        for (auto itdi = iti->begin(); itdi != iti->end(); itdi++) {
            auto const &die1 = *itdi;
            if (!die1->die()->isStopped()) {
                // first calculate bounce for the rest of the current list of dice
                auto itdj = itdi;
                itdj++;
                while (itdj != iti->end()) {
                    auto const &die2 = *(itdj);
                    if (!die2->die()->isStopped()) {
                        die1->die()->calculateBounce(die2->die().get());
                    }
                    itdj++;
                }

                // next, calculate the bounce for the next lists of dice.
                auto itj = iti;
                itj++;
                while (itj != m_dice.end()) {
                    for (auto const &die2 : *itj) {
                        if (!die2->die()->isStopped()) {
                            die1->die()->calculateBounce(die2->die().get());
                        }
                    }
                    itj++;
                }
            }
        }
    }

    bool needsRedraw = false;
    uint32_t i = 0;
    float width = m_screenWidth;
    float height = m_screenHeight;
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            if (die->die()->updateModelMatrix()) {
                needsRedraw = true;
            }

            // check if the stopped animation is done and not started because if the dice are just
            // positioned, then the stopped animation is done, but never started.
            if (die->die()->isStopped() && !die->die()->isStoppedAnimationDone() &&
                !die->die()->isStoppedAnimationStarted()) {
                auto nbrX = static_cast<uint32_t>(width / (2 * DicePhysicsModel::stoppedRadius));
                uint32_t stoppedX = i % nbrX;
                uint32_t stoppedY = i / nbrX;
                float x = -width / 2 + (2 * stoppedX + 1) * DicePhysicsModel::stoppedRadius;
                float y = height / 2 - (2 * stoppedY + 1) * DicePhysicsModel::stoppedRadius;
                die->die()->animateMove(x, y);
            }
            i++;
        }
    }

    return needsRedraw;
}

template <typename DiceType>
std::vector<std::vector<uint32_t >> RainbowDiceGraphics<DiceType>::getDiceResults() {
    std::vector<std::vector<uint32_t>> results;
    for (auto const &dice : m_dice) {
        std::vector<uint32_t > dieResults;
        for (auto const &die: dice) {
            if (!die->die()->isStopped()) {
                // Should not happen
                throw std::runtime_error("Not all die are stopped!");
            }
            dieResults.push_back(die->die()->getResult());
        }
        results.push_back(dieResults);
    }

    return std::move(results);
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::resetToStoppedPositions(std::vector<std::vector<uint32_t>> const &upFaceIndices) {
    uint32_t i = 0;
    uint32_t k = 0;
    float width = m_screenWidth;
    float height = m_screenHeight;
    auto nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));

    for (auto diceIt = m_dice.begin(); diceIt != m_dice.end(); diceIt++) {
        size_t size = diceIt->size();
        while (diceIt != m_dice.end() && upFaceIndices[i].empty()) {
            diceIt->clear();
            i++;
            diceIt++;
        }
        if (diceIt == m_dice.end()) {
            break;
        }
        for (uint32_t j = 0; j < upFaceIndices[i].size(); j++) {
            uint32_t stoppedX = k%nbrX;
            uint32_t stoppedY = k/nbrX;
            float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;

            if (j < size) {
                auto diceItj = diceIt->begin();
                for (int l = 0; l < j; l++) {
                    diceItj++;
                }
                diceItj->get()->die()->positionDice(upFaceIndices[i][j], x, y);
            } else {
                auto die = createDie(*(diceIt->begin()));
                die->die()->positionDice(upFaceIndices[i][j], x, y);
                diceIt->push_back(die);
            }
            k++;
        }
        i++;
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::setDice(std::string const &inDiceName,
                                            std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions,
                                            bool inIsModifiedRoll) {
    RainbowDice::setDice(inDiceName, inDiceDescriptions, inIsModifiedRoll);

    m_dice.clear();
    for (auto const &diceDescription : m_diceDescriptions) {
        for (int i = 0; i < diceDescription->m_nbrDice; i++) {
            loadObject(diceDescription->m_symbols, diceDescription->m_rerollOnIndices,
                       diceDescription->m_color);
        }
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::addRerollDice(bool resetPosition) {
    for (auto &dice : m_dice) {
        if (dice.empty()) {
            continue;
        }
        // only check the last dice in the list to see if it needs to be rerolled.  The other dice
        // in the list already got rerolled.  That is why there is a die after it.
        auto const &die = *(dice.rbegin());

        uint32_t result = die->die()->getResult() % die->die()->getNumberOfSymbols();
        bool shouldReroll = false;
        for (auto const &rerollIndex : die->rerollIndices()) {
            if (result == rerollIndex) {
                shouldReroll = true;
                break;
            }
        }

        if (!shouldReroll) {
            continue;
        }

        auto dieNew = createDie(die);
        if (resetPosition) {
            dieNew->die()->resetPosition();
        }

        dice.push_back(dieNew);
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::addRollingDice() {
    addRerollDice(true);
    animateMoveStoppedDice();
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::moveDiceToStoppedRandomUpface() {
    int i=0;
    auto nbrX = static_cast<uint32_t>(m_screenWidth / (2 * DicePhysicsModel::stoppedRadius));
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            uint32_t stoppedX = i % nbrX;
            uint32_t stoppedY = i / nbrX;
            float x = -m_screenWidth / 2 + (2 * stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = m_screenHeight / 2 - (2 * stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die()->positionDice(x, y, true);
            i++;
        }
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::moveDiceToStoppedPositions() {
    int i=0;
    auto nbrX = static_cast<uint32_t>(m_screenWidth / (2 * DicePhysicsModel::stoppedRadius));
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            uint32_t stoppedX = i % nbrX;
            uint32_t stoppedY = i / nbrX;
            float x = -m_screenWidth / 2 + (2 * stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = m_screenHeight / 2 - (2 * stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die()->positionDice(x, y, false);
            i++;
        }
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::animateMoveStoppedDice() {
    int i=0;
    auto nbrX = static_cast<uint32_t>(m_screenWidth / (2 * DicePhysicsModel::stoppedRadius));
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            if (die->die()->isStopped()) {
                uint32_t stoppedX = i % nbrX;
                uint32_t stoppedY = i / nbrX;
                float x = -m_screenWidth / 2 + (2 * stoppedX + 1) * DicePhysicsModel::stoppedRadius;
                float y = m_screenHeight / 2 - (2 * stoppedY + 1) * DicePhysicsModel::stoppedRadius;
                die->die()->animateMove(x, y);
            }
            i++;
        }
    }
}

// returns true if a result is ready, false otherwise
template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::rerollSelected() {
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            if (die->isSelected()) {
                die->die()->resetPosition();
                die->toggleSelected();
                m_isModifiedRoll = true;
            }
        }
    }
    if (!m_drawRollingDice) {
        moveDiceToStoppedPositions();
        return true;
    } else {
        return false;
    }
}

// returns true if a result is ready, false otherwise
template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::addRerollSelected() {
    for (auto &dice : m_dice) {
        for (auto diceIt = dice.begin(); diceIt != dice.end(); diceIt++) {
            if (diceIt->get()->isSelected()) {
                m_isModifiedRoll = true;

                auto die = createDie(*diceIt);
                die->die()->resetPosition();
                diceIt->get()->toggleSelected();

                diceIt++;
                dice.insert(diceIt, die);
                diceIt--;
            }
        }
    }

    if (m_drawRollingDice) {
        // animate the rolling dice
        animateMoveStoppedDice();
        return false;
    } else {
        // just display the result
        moveDiceToStoppedPositions();
        return true;
    }
}

// returns true if a redraw is needed, and false otherwise.
template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::deleteSelected() {
    bool diceDeleted = false;
    for (auto &dice : m_dice) {
        for (auto diceIt = dice.begin(); diceIt != dice.end();) {
            if (diceIt->get()->isSelected()) {
                m_isModifiedRoll = true;
                diceDeleted = true;
                diceIt = dice.erase(diceIt);
            } else {
                diceIt++;
            }
        }
    }

    if (diceDeleted) {
        if (m_drawRollingDice) {
            animateMoveStoppedDice();
        } else {
            moveDiceToStoppedPositions();
        }
    }

    return diceDeleted;
}

#endif // RAINBOWDICE_HPP