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

#ifndef RAINBOWDICE_TEXTUREATLASGL_HPP
#define RAINBOWDICE_TEXTUREATLASGL_HPP

#include <memory>
#include <GLES2/gl2.h>
#include "text.hpp"

class TextureGL {
public:
    explicit TextureGL(std::shared_ptr<TextureAtlas> inTextureAtlas) :
        m_textureAtlas{std::move(inTextureAtlas)}
    {
        initGLResources();
    }

    void destroyGLResources() {
        glDeleteTextures(1, &m_texture);
    }

    void initGLResources() {
        // load the textures
        glGenTextures(1, &m_texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        // when sampling outside of the texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // when the texture is scaled up or down
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_LINEAR*/ GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureAtlas->getImageWidth(),
                     m_textureAtlas->getImageHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     m_textureAtlas->bitmap().get());

        glGenerateMipmap(GL_TEXTURE_2D);
    }

    ~TextureGL() {
        destroyGLResources();
    }

    inline GLuint texture() { return m_texture; }

    inline std::shared_ptr<TextureAtlas> const &textureAtlas() { return m_textureAtlas; }

private:
    std::shared_ptr<TextureAtlas> m_textureAtlas;
    GLuint m_texture;
};

#endif /* RAINBOWDICE_TEXTUREATLASGL_HPP */