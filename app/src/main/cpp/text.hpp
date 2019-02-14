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
#ifndef TEXT_HPP
#define TEXT_HPP
#include <string>
#include <map>
#include <vector>

struct TextureImage {
    float left;
    float right;
    float top;
    float bottom;
};

/* texture image */
typedef std::map<std::string, TextureImage>::iterator texture_iterator;

class TextureAtlas {
protected:
    uint32_t width;
    uint32_t height;
    std::map<std::string, TextureImage> textureImages;

public:
    uint32_t getImageWidth() { return width; }
    uint32_t getImageHeight() {return height; }
    uint32_t getNbrImages() {
        return (uint32_t)textureImages.size();
    }

    virtual TextureImage getTextureCoordinates(std::string const &symbol) {
        std::map<std::string, TextureImage>::iterator it = textureImages.find(symbol);
        if (it == textureImages.end()) {
            // shouldn't happen
            throw std::runtime_error(std::string("Texture not found for symbol: ") + symbol);
        }

        return it->second;
    }

    TextureAtlas(std::vector<std::string> const &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                 std::vector<std::pair<float, float>> const &inLeftRightTextureCoordinate,
                 std::vector<std::pair<float, float>> const &inTopBottomTextureCoordinate)
        :width(inWidth), height(inHeightTexture), textureImages()
    {
        for (uint32_t i=0; i < symbols.size(); i++) {
            TextureImage tex = { inLeftRightTextureCoordinate[i].first,
                                 inLeftRightTextureCoordinate[i].second,
                                 inTopBottomTextureCoordinate[i].first,
                                 inTopBottomTextureCoordinate[i].second };
            textureImages.insert(std::make_pair(symbols[i], tex));
        }
    }

    virtual ~TextureAtlas() {}
};
#endif
