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

        if (m_isBeingRerolled) {
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
    inline bool isBeingRerolled() { return m_isBeingRerolled; }
    inline std::vector<uint32_t> const &rerollIndices() { return m_rerollIndices; }
    inline bool isSelected() { return m_isSelected; }
    inline size_t nbrIndices() { return m_die->getIndices().size(); }
    inline void setIsBeingRerolled(bool inIsBeingReRolled) { m_isBeingRerolled = inIsBeingReRolled; }

    DiceGraphics(std::vector<std::string> const &symbols, std::vector<uint32_t> inRerollIndices,
                 std::vector<float> const &color,
                 std::shared_ptr<TextureAtlas> const &textureAtlas)
            : m_die{std::move(createDice(symbols, color))},
              m_isBeingRerolled{false},
              m_rerollIndices{std::move(inRerollIndices)},
              m_isSelected{false},
              m_vertexBuffer{},
              m_indexBuffer{}

    {
        m_die->loadModel(textureAtlas);
        m_die->resetPosition(screenWidth, screenHeight);
    }

    virtual ~DiceGraphics() = default;
protected:
    std::shared_ptr<DicePhysicsModel> m_die;
    bool m_isBeingRerolled;
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
    virtual void initModels()=0;

    virtual void initThread()=0;

    virtual void drawFrame()=0;

    virtual bool updateUniformBuffer()=0;

    virtual bool allStopped()=0;

    virtual std::vector<std::vector<uint32_t>> getDiceResults()=0;

    virtual bool needsReroll()=0;

    virtual void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &rerollSymbol,
                            std::vector<float> const &color)=0;

    virtual void recreateSwapChain(uint32_t width, uint32_t height)=0;

    virtual void updateAcceleration(float x, float y, float z)=0;

    virtual void resetPositions()=0;

    virtual void resetPositions(std::set<int> &dice)=0;

    virtual void resetToStoppedPositions(std::vector<std::vector<uint32_t>> const &inUpFaceIndices)=0;

    virtual void addRollingDice() = 0;

    virtual void cleanupThread() = 0;

    virtual void setTexture(std::shared_ptr<TextureAtlas> texture) = 0;

    // returns true if it needs a redraw
    virtual bool tapDice(float x, float y) = 0;

    virtual void rerollSelected() = 0;
    virtual void addRerollSelected() = 0;

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
    }

    virtual void resetView() {
        m_viewPoint = startViewPoint();
        m_viewPointCenterPosition = startViewPointCenterPosition();
        setView();
    }

    virtual void setDice(std::string const &inDiceName,
            std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescription,
            bool inIsModified = false) {
        m_diceName = inDiceName;
        m_diceDescriptions = inDiceDescription;
        m_isModifiedRoll = inIsModified;
    }

    std::vector<std::shared_ptr<DiceDescription>> const &getDiceDescriptions() {
        return m_diceDescriptions;
    }

    std::string diceName() {
        return m_diceName;
    }

    bool isModifiedRoll() { return m_isModifiedRoll; }

    RainbowDice()
            : m_viewPoint{startViewPoint()},
              m_viewPointCenterPosition{startViewPointCenterPosition()},
              m_isModifiedRoll{false}
    {}

    virtual ~RainbowDice() = default;

protected:
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
            bool inIsModifiedRoll = false) override;
    void rerollSelected() override;
    void addRerollSelected() override;

    void loadObject(std::vector<std::string> const &symbols,
                                       std::vector<uint32_t> const &rerollIndices,
                                       std::vector<float> const &color) override {
        m_dice.push_back(createDie(symbols, rerollIndices, color));
    }

    void updateAcceleration(float x, float y, float z) override {
        for (auto const &die : m_dice) {
            die->die()->updateAcceleration(x, y, z);
        }
    }

    void resetPositions() override {
        float width = screenWidth;
        float height = screenHeight;
        for (auto const &die: m_dice) {
            die->die()->resetPosition(width, height);
        }
    }

    void resetPositions(std::set<int> &diceIndices) override {
        int i = 0;
        float width = screenWidth;
        float height = screenHeight;
        for (auto const &die : m_dice) {
            if (diceIndices.find(i) != diceIndices.end()) {
                die->die()->resetPosition(width, height);
            }
            i++;
        }
    }

    bool needsReroll() override {
        for (auto const &die : m_dice) {
            if (die->needsReroll()) {
                return true;
            }
        }

        return false;
    }

    bool allStopped() override {
        for (auto &&die : m_dice) {
            if (!die->die()->isStopped() || !die->die()->isStoppedAnimationDone()) {
                return false;
            }
        }
        return true;
    }

    RainbowDiceGraphics()
      : RainbowDice{},
        m_dice{}
    {}

    ~RainbowDiceGraphics() override = default;
protected:
    using DiceList = std::list<std::shared_ptr<DiceType>>;
    DiceList m_dice;

    virtual std::shared_ptr<DiceType> createDie(std::vector<std::string> const &symbols,
                                                std::vector<uint32_t> const &inRerollIndices,
                                                std::vector<float> const &color) = 0;
    virtual std::shared_ptr<DiceType> createDie(std::shared_ptr<DiceType> const &inDice) = 0;
    virtual bool invertY() = 0;
};

// Template class functions

template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::tapDice(float x, float y, uint32_t width, uint32_t height) {
    bool needsRedraw = false;
    float swidth = screenWidth;
    float sheight = screenHeight;
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
    for (auto const &die : m_dice) {
        float distanceToLine = glm::length(glm::cross(line, die->die()->position() - start));
        if (distanceToLine < DicePhysicsModel::stoppedRadius) {
            die->toggleSelected();
            needsRedraw = true;
            break;
        }
    }

    return needsRedraw;
}

template <typename DiceType>
bool RainbowDiceGraphics<DiceType>::updateUniformBuffer() {
    for (auto iti = m_dice.begin(); iti != m_dice.end(); iti++) {
        if (!iti->get()->die()->isStopped()) {
            for (auto itj = iti; itj != m_dice.end(); itj++) {
                if (!itj->get()->die()->isStopped() && iti != itj) {
                    iti->get()->die()->calculateBounce(itj->get()->die().get());
                }
            }
        }
    }

    bool needsRedraw = false;
    uint32_t i = 0;
    float width = screenWidth;
    float height = screenHeight;
    for (auto &&die : m_dice) {
        if (die->die()->updateModelMatrix()) {
            needsRedraw = true;
        }

        // check if the stopped animation is done and not started because if the dice are just
        // positioned, then the stopped animation is done, but never started.
        if (die->die()->isStopped() && !die->die()->isStoppedAnimationDone() &&
                !die->die()->isStoppedAnimationStarted()) {
            auto nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die()->animateMove(x, y);
        }
        i++;
    }

    return needsRedraw;
}

template <typename DiceType>
std::vector<std::vector<uint32_t >> RainbowDiceGraphics<DiceType>::getDiceResults() {
    std::vector<std::vector<uint32_t>> results;
    for (auto dieIt = m_dice.begin(); dieIt != m_dice.end(); dieIt++) {
        if (!dieIt->get()->die()->isStopped()) {
            // Should not happen
            throw std::runtime_error("Not all die are stopped!");
        }
        std::vector<uint32_t > dieResults;
        while (dieIt->get()->isBeingRerolled()) {
            dieResults.push_back(dieIt->get()->die()->getResult());
            dieIt++;
        }
        dieResults.push_back(dieIt->get()->die()->getResult());
        results.push_back(dieResults);
    }

    return results;
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::resetToStoppedPositions(std::vector<std::vector<uint32_t>> const &upFaceIndices) {
    uint32_t i = 0;
    uint32_t k = 0;
    float width = screenWidth;
    float height = screenHeight;
    auto nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
    for (auto diceIt = m_dice.begin(); diceIt != m_dice.end(); diceIt++) {
        for (uint32_t j = 0; j < upFaceIndices[i].size(); j++) {
            uint32_t stoppedX = k%nbrX;
            uint32_t stoppedY = k/nbrX;
            float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;

            if (j == 0) {
                diceIt->get()->die()->positionDice(upFaceIndices[i][j], x, y);
            } else {
                auto die = createDie(*diceIt);
                die->die()->positionDice(upFaceIndices[i][j], x, y);
                diceIt->get()->setIsBeingRerolled(true);
                diceIt++;
                m_dice.insert(diceIt, die);
                diceIt--;
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
void RainbowDiceGraphics<DiceType>::addRollingDice() {
    float width = screenWidth;
    float height = screenHeight;
    for (auto diceIt = m_dice.begin(); diceIt != m_dice.end(); diceIt++) {
        // Skip dice already being rerolled or does not get rerolled.
        if (diceIt->get()->isBeingRerolled() || diceIt->get()->rerollIndices().empty()) {
            continue;
        }

        uint32_t result = diceIt->get()->die()->getResult() % diceIt->get()->die()->getNumberOfSymbols();
        bool shouldReroll = false;
        for (auto const &rerollIndex : diceIt->get()->rerollIndices()) {
            if (result == rerollIndex) {
                shouldReroll = true;
                break;
            }
        }

        if (!shouldReroll) {
            continue;
        }

        auto die = createDie(*diceIt);
        diceIt->get()->setIsBeingRerolled(true);

        diceIt++;
        m_dice.insert(diceIt, die);
        diceIt--;
    }

    int i=0;
    for (auto const &die : m_dice) {
        if (die->die()->isStopped()) {
            auto nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die()->animateMove(x, y);
        }
        i++;
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::rerollSelected() {
    for (auto const &die : m_dice) {
        if (die->isSelected()) {
            die->die()->resetPosition(screenWidth, screenHeight);
            die->toggleSelected();
            m_isModifiedRoll = true;
        }
    }
}

template <typename DiceType>
void RainbowDiceGraphics<DiceType>::addRerollSelected() {
    for (auto diceIt = m_dice.begin(); diceIt != m_dice.end(); diceIt++) {
        if (diceIt->get()->isSelected()) {
            m_isModifiedRoll = true;

            auto die = createDie(*diceIt);
            diceIt->get()->toggleSelected();

            if (diceIt->get()->isBeingRerolled()) {
                die->setIsBeingRerolled(true);
            } else {
                diceIt->get()->setIsBeingRerolled(true);
            }

            diceIt++;
            m_dice.insert(diceIt, die);
            diceIt--;
        }
    }

    int i=0;
    for (auto const &die : m_dice) {
        if (die->die()->isStopped()) {
            auto nbrX = static_cast<uint32_t>(screenWidth/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -screenWidth/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = screenHeight/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die()->animateMove(x, y);
        }
        i++;
    }
}

#endif // RAINBOWDICE_HPP