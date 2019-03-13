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

        inline uint32_t width() { return m_width; }
        inline uint32_t height() { return m_height; }
        inline EGLSurface surface() { return m_surface; }
        inline EGLDisplay display() { return m_display; }
        inline std::shared_ptr<WindowType> window() { return m_window; }

        ~Surface() {
            destroySurface();
        }

    private:
        std::shared_ptr<WindowType> m_window;
        EGLContext m_context;
        EGLConfig m_config;
        EGLSurface m_surface;
        EGLDisplay m_display;

        uint32_t m_width;
        uint32_t m_height;

        void createSurface();
        void destroySurface();
    };
} /* namespace graphicsGL */

struct GLGraphics {
    using Buffer = GLuint;
};

struct DiceGL : DiceGraphics<GLGraphics> {
    DiceGL(std::vector<std::string> const &symbols, std::vector<uint32_t> inRerollIndices,
           std::vector<float> const &color, std::shared_ptr<TextureAtlas> const &inTextureAtlas)
            : DiceGraphics{symbols, std::move(inRerollIndices), color, inTextureAtlas}
    {
        createGLResources();
    }

    void destroyGLResources() {
        glDeleteBuffers(1, &m_vertexBuffer);
        glDeleteBuffers(1, &m_indexBuffer);
    }

    void createGLResources() {
        // the vertex buffer
        glGenBuffers(1, &m_vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_die->getVertices().size(),
                     m_die->getVertices().data(), GL_STATIC_DRAW);

        // the index buffer
        glGenBuffers(1, &m_indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * m_die->getIndices().size(),
                     m_die->getIndices().data(), GL_STATIC_DRAW);
    }

    ~DiceGL() override {
        destroyGLResources();
    }

private:
};

class RainbowDiceGL : public RainbowDiceGraphics<DiceGL> {
public:
    explicit RainbowDiceGL(std::shared_ptr<WindowType> window)
            : RainbowDiceGraphics{},
              m_surface{std::make_shared<graphicsGL::Surface>(std::move(window))},
              programID{0}
    {
        init();
        setView();
        updatePerspectiveMatrix(m_surface->width(), m_surface->height());
    }

    void initModels() override;

    void initThread() override { m_surface->initThread(); }

    void cleanupThread() override { m_surface->cleanupThread(); }

    void drawFrame() override;

    void recreateModels();

    void recreateSwapChain(uint32_t width, uint32_t height) override;

    bool tapDice(float x, float y) override {
        return RainbowDiceGraphics::tapDice(x, y, m_surface->width(), m_surface->height());
    }

    void scroll(float distanceX, float distanceY) override {
        RainbowDice::scroll(distanceX, distanceY, m_surface->width(), m_surface->height());
    }

    void setTexture(std::shared_ptr<TextureAtlas> inTexture) override {
        m_texture = std::make_shared<TextureGL>(std::move(inTexture));
    }

    void destroyModelGLResources();

    void destroyGLResources() {
        glDeleteProgram(programID);
    }

    ~RainbowDiceGL() override {
        destroyGLResources();
    }
protected:
    bool invertY() override { return true; }

    std::shared_ptr<DiceGL> createDie(std::vector<std::string> const &symbols,
                                      std::vector<uint32_t> const &inRerollIndices,
                                      std::vector<float> const &color) override;
    std::shared_ptr<DiceGL> createDie(std::shared_ptr<DiceGL> const &inDice) override;
private:
    std::shared_ptr<graphicsGL::Surface> m_surface;
    GLuint programID;
    std::shared_ptr<TextureGL> m_texture;

    GLuint loadShaders(std::string const &vertexShaderFile, std::string const &fragmentShaderFile);
    void init();
};