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
#ifndef RAINBOWDICE_TEXT_HPP
#define RAINBOWDICE_TEXT_HPP
#include <string>
#include <map>
#include <vector>

struct TextureImage {
    float left;
    float right;
    float top;
    float bottom;
};

class TextureAtlas {
protected:
    uint32_t m_width;
    uint32_t m_height;
    std::map<std::string, TextureImage> m_textureImages;
    std::unique_ptr<unsigned char[]> m_bitmap;
    uint32_t m_bitmapLength;

public:
    TextureAtlas(std::vector<std::string> const &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                 std::vector<std::pair<float, float>> const &inLeftRightTextureCoordinate,
                 std::vector<std::pair<float, float>> const &inTopBottomTextureCoordinate,
                 std::unique_ptr<unsigned char[]> &&inBitmap, uint32_t inBitmapLength)
            : m_width(inWidth), m_height(inHeightTexture), m_textureImages(),
              m_bitmap{std::move(inBitmap)}, m_bitmapLength{inBitmapLength}
    {
        for (uint32_t i=0; i < symbols.size(); i++) {
            TextureImage tex = { inLeftRightTextureCoordinate[i].first,
                                 inLeftRightTextureCoordinate[i].second,
                                 inTopBottomTextureCoordinate[i].first,
                                 inTopBottomTextureCoordinate[i].second };
            m_textureImages.insert(std::make_pair(symbols[i], tex));
        }
    }

    uint32_t getImageWidth() { return m_width; }
    uint32_t getImageHeight() {return m_height; }
    uint32_t getNbrImages() {
        return (uint32_t)m_textureImages.size();
    }

    TextureImage getTextureCoordinates(std::string const &symbol) {
        auto it = m_textureImages.find(symbol);
        if (it == m_textureImages.end()) {
            // shouldn't happen
            throw std::runtime_error(std::string("Texture not found for symbol: ") + symbol);
        }

        return it->second;
    }

    std::unique_ptr<unsigned char[]> const &bitmap() {
        return m_bitmap;
    }

    uint32_t bitmapLength() {
        return m_bitmapLength;
    }

    ~TextureAtlas() = default;
};
#endif
