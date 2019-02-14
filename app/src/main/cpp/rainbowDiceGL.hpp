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
        Surface(WindowType *window)
                : m_window{window},
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

        ~Surface() {
            destroySurface();
        }

    private:
        WindowType *m_window;
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

    DiceGL(std::vector<std::string> const &symbols, std::vector<uint32_t> const &inRerollIndices,
           glm::vec3 &position, std::vector<float> const &color)
            : die(nullptr),
              rerollIndices{inRerollIndices}
    {
        initDice(symbols, position, color);
    }

    void loadModel(int width, int height, std::shared_ptr<TextureAtlas> const &texAtlas) {
        die->loadModel(texAtlas);
        die->setView();
        die->updatePerspectiveMatrix(width, height);
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
        long nbrSides = symbols.size();
        if (nbrSides == 2) {
            die.reset(new DiceModelCoin(symbols, position, color, true));
        } else if (nbrSides == 4) {
            die.reset(new DiceModelTetrahedron(symbols, position, color, true));
        } else if (nbrSides == 12) {
            die.reset(new DiceModelDodecahedron(symbols, position, color, true));
        } else if (nbrSides == 20) {
            die.reset(new DiceModelIcosahedron(symbols, position, color, true));
        } else if (nbrSides == 30) {
            die.reset(new DiceModelRhombicTriacontahedron(symbols, position, color, true));
        } else if (6 % nbrSides == 0) {
            die.reset(new DiceModelCube(symbols, position, color, true));
        } else {
            die.reset(new DiceModelHedron(symbols, position, color, nbrSides, true));
        }
    }
};

class RainbowDiceGL : public RainbowDice {
public:
    RainbowDiceGL(WindowType *window, std::vector<std::string> &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                  std::vector<std::pair<float, float>> const &textureCoordsLeftRight,
                  std::vector<std::pair<float, float>> const &textureCoordsTopBottom,
                  std::unique_ptr<unsigned char[]> const &inBitmap)
            : m_surface{window},
              m_textureAtlas{new TextureAtlasGL{symbols, inWidth, inHeightTexture, textureCoordsLeftRight,
                                                textureCoordsTopBottom, inBitmap}}
    {
        init();
    }

    virtual void initModels();

    virtual void initThread() { m_surface.initThread(); }

    virtual void cleanupThread() { m_surface.cleanupThread(); }

    virtual void drawFrame();

    virtual bool updateUniformBuffer();

    virtual bool allStopped();

    virtual std::vector<std::vector<uint32_t >> getDiceResults();

    virtual bool needsReroll();

    virtual void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &inRerollIndices,
                            std::vector<float> const &color);

    virtual void recreateModels();

    virtual void recreateSwapChain();

    virtual void updateAcceleration(float x, float y, float z);

    virtual void resetPositions();

    virtual void resetPositions(std::set<int> &dice);

    virtual void resetToStoppedPositions(std::vector<uint32_t > const &inUpFaceIndices);

    virtual void addRollingDice();

    virtual ~RainbowDiceGL() {
    }
private:
    graphicsGL::Surface m_surface;
    GLuint programID;
    std::shared_ptr<TextureAtlasGL> m_textureAtlas;

    typedef std::list<std::shared_ptr<DiceGL> > DiceList;
    DiceList dice;

    GLuint loadShaders(std::string const &vertexShaderFile, std::string const &fragmentShaderFile);
    void init();
};