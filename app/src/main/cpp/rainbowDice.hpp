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
#include <vector>
#include <set>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "rainbowDiceGlobal.hpp"
#include "diceDescription.hpp"
#include "text.hpp"

class RainbowDice {
protected:
    std::string m_diceName;
    std::vector<std::shared_ptr<DiceDescription>> m_diceDescriptions;

    glm::mat4 m_proj;
    glm::mat4 m_view;

    glm::vec3 m_viewPoint;
    glm::vec3 m_viewPointCenterPosition;

    static constexpr float m_maxZ = 10.0f;
    static constexpr float m_minZ = 1.5f;
    static constexpr float m_maxScroll = 10.0f;

public:
    RainbowDice()
        : m_viewPoint{0.0f, 0.0f, 3.0f},
          m_viewPointCenterPosition{0.0f, 0.0f, 0.0f}
    {}

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

    virtual void recreateModels()=0;

    virtual void recreateSwapChain(uint32_t width, uint32_t height)=0;

    virtual void updateAcceleration(float x, float y, float z)=0;

    virtual void resetPositions()=0;

    virtual void resetPositions(std::set<int> &dice)=0;

    virtual void resetToStoppedPositions(std::vector<uint32_t > const &inUpFaceIndices)=0;

    virtual void addRollingDice()=0;

    virtual void cleanupThread()=0;

    virtual void setTexture(std::shared_ptr<TextureAtlas> texture) = 0;

    virtual void newSurface(std::shared_ptr<WindowType> surface) = 0;

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

    inline float screenWidth(uint32_t width, uint32_t height) {
        //float avg = (width + height)/2.0f;
        //return width/avg*2.0f;
        //if (height > width) {
        //    return 2.0f;
        //} else {
        //    return width/ (float) height *2.0f;
        //}

        return 2.0f;
    }

    inline float screenHeight(uint32_t width, uint32_t height) {
        //float avg = (width + height)/2.0f;
        //return height/avg*2.0f;
        //if (height > width) {
        //    return height/(float) width;
        //} else {
        //    return 2.0f;
        //}

        return 2.0f;
    }

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

    virtual void setDice(std::string const &inDiceName, std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescription) {
        m_diceName = inDiceName;
        m_diceDescriptions = inDiceDescription;
    }

    std::vector<std::shared_ptr<DiceDescription>> const &getDiceDescriptions() {
        return m_diceDescriptions;
    }

    std::string diceName() {
        return m_diceName;
    }

    virtual ~RainbowDice() = default;
};
#endif // RAINBOWDICE_HPP