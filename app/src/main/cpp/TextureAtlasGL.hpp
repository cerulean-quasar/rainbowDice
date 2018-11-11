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

#ifndef RAINBOWDICE_TEXTUREATLASGL_HPP
#define RAINBOWDICE_TEXTUREATLASGL_HPP

#include <GLES2/gl2.h>
#include "text.hpp"

class TextureAtlasGL : public TextureAtlas {
public:
    TextureAtlasGL(std::vector<std::string> &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                   uint32_t inHeightImage, uint32_t inHeightBlankSpace, std::vector<char> const &inBitmap)
        : TextureAtlas{symbols, inWidth, inHeightTexture, inHeightImage, inHeightBlankSpace},
          m_bitmap{inBitmap}
    {
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

        std::vector<char> &texture = m_bitmap;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width,
                     heightTexture, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_bitmap.data());

        glGenerateMipmap(GL_TEXTURE_2D);

    }

    ~TextureAtlasGL() {
        glDeleteTextures(1, &m_texture);
    }

    std::vector<char> &image() { return m_bitmap; }
    inline GLuint texture() { return m_texture; }

private:
    GLuint m_texture;
    std::vector<char> m_bitmap;
};

#endif /* RAINBOWDICE_TEXTUREATLASGL_HPP */