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

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <list>
#include "rainbowDice.hpp"
#include "dice.hpp"
#include "rainbowDiceGlobal.hpp"
#include "TextureAtlasGL.hpp"

namespace graphicsGL {
    class Surface {
    public:
        explicit Surface(std::shared_ptr<WindowType> window)
                : m_window{std::move(window)},
                  m_context{EGL_NO_CONTEXT},
                  m_config{},
                  m_surface{EGL_NO_SURFACE},
                  m_display{EGL_NO_DISPLAY},
                  m_width{0},
                  m_height{0}
        {
            createSurface();
        }

        void initThread();
        void cleanupThread();

        inline int width() { return m_width; }
        inline int height() { return m_height; }
        inline EGLSurface surface() { return m_surface; }
        inline EGLDisplay display() { return m_display; }

        void reloadSurfaceDimensions();

        ~Surface() {
            destroySurface();
        }

    private:
        std::shared_ptr<WindowType> m_window;
        EGLContext m_context;
        EGLConfig m_config;
        EGLSurface m_surface;
        EGLDisplay m_display;

        int m_width;
        int m_height;

        void createSurface();
        void destroySurface();
    };
} /* namespace graphicsGL */

struct DiceGL {
    GLuint vertexBuffer;
    GLuint indexBuffer;
    std::shared_ptr<DicePhysicsModel> die;

    // If the die is being rerolled, then don't count it in the results list returned because
    // the GUI is expecting a new list of results that only contains one set of rolls for a die
    // and then uses the index to determine what the reroll value was.
    bool isBeingReRolled;

    std::vector<uint32_t> rerollIndices;

    DiceGL(std::vector<std::string> const &symbols, std::vector<uint32_t> inRerollIndices,
           glm::vec3 &position, std::vector<float> const &color)
            : vertexBuffer{0},
              indexBuffer{0},
              die{},
              isBeingReRolled{false},
              rerollIndices{std::move(inRerollIndices)}
    {
        initDice(symbols, position, color);
    }

    void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
        die->loadModel(texAtlas);
    }

    bool needsReroll() {
        if (!die->isStopped()) {
            // should not happen!
            return false;
        }

        if (isBeingReRolled) {
            return false;
        }

        uint32_t result = die->getResult();

        for (auto const & rerollIndex : rerollIndices) {
            if (result == rerollIndex) {
                return true;
            }
        }

        return false;
    }

    ~DiceGL() {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
    }

private:
    void initDice(std::vector<std::string> const &symbols, glm::vec3 &position,
                  std::vector<float> const &color) {
        isBeingReRolled = false;
        die = std::move(createDice(symbols, color, true));
    }
};

class RainbowDiceGL : public RainbowDice {
public:
    explicit RainbowDiceGL(std::shared_ptr<WindowType> window)
            : RainbowDice{},
              m_surface{std::move(window)},
              programID{0}
    {
        init();
        setView();
        updatePerspectiveMatrix(m_surface.width(), m_surface.height());
    }

    void initModels() override;

    void initThread() override { m_surface.initThread(); }

    void cleanupThread() override { m_surface.cleanupThread(); }

    void drawFrame() override;

    bool updateUniformBuffer() override;

    bool allStopped() override;

    std::vector<std::vector<uint32_t >> getDiceResults() override;

    bool needsReroll() override;

    void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &inRerollIndices,
                            std::vector<float> const &color) override;

    void recreateModels() override;

    void recreateSwapChain(uint32_t width, uint32_t height) override;

    void updateAcceleration(float x, float y, float z) override;

    void resetPositions() override;

    void resetPositions(std::set<int> &dice) override;

    void resetToStoppedPositions(std::vector<uint32_t > const &inUpFaceIndices) override;

    void addRollingDice() override;

    void newSurface(std::shared_ptr<WindowType> surface) override {
        // TODO: do something here
    }

    void scroll(float distanceX, float distanceY) override {
        RainbowDice::scroll(distanceX, distanceY, m_surface.width(), m_surface.height());
    }

    void setTexture(std::shared_ptr<TextureAtlas> inTexture) override {
        m_texture = std::make_shared<TextureGL>(std::move(inTexture));
    }

    void setDice(std::string const &inDiceName,
            std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescription) override {
        RainbowDice::setDice(inDiceName, inDiceDescription);

        dice.clear();
        for (auto const &diceDescription : m_diceDescriptions) {
            for (int i = 0; i < diceDescription->m_nbrDice; i++) {
                loadObject(diceDescription->m_symbols, diceDescription->m_rerollOnIndices,
                           diceDescription->m_color);
            }
        }

    }

    ~RainbowDiceGL() override = default;
private:
    graphicsGL::Surface m_surface;
    GLuint programID;
    std::shared_ptr<TextureGL> m_texture;

    typedef std::list<std::shared_ptr<DiceGL> > DiceList;
    DiceList dice;

    GLuint loadShaders(std::string const &vertexShaderFile, std::string const &fragmentShaderFile);
    void init();
};