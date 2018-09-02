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
    uint32_t imageIndex;
};

/* texture image */
typedef std::map<std::string, TextureImage>::iterator texture_iterator;

class TextureAtlas {
private:
    uint32_t heightImage;
    uint32_t heightBlankSpace;
    uint32_t width;
    uint32_t heightTexture;
    std::vector<char> bitmap;
    std::map<std::string, TextureImage> textureImages;

public:
    std::vector<char> &getImage() { return bitmap; }
    uint32_t getImageHeight() { return heightImage; }
    uint32_t getImageWidth() { return width; }
    uint32_t getTextureHeight() {return heightTexture; }
    uint32_t getPaddingHeight() {return heightBlankSpace; }
    uint32_t getImageIndex(std::string &symbol) {
        std::map<std::string, TextureImage>::iterator it = textureImages.find(symbol);
        if (it == textureImages.end()) {
            // shouldn't happen
            throw std::runtime_error(std::string("Texture not found for symbol: ") + symbol);
        }

        return it->second.imageIndex;
    }
    uint32_t getNbrImages() {
        return (uint32_t)textureImages.size();
    }

    TextureAtlas(std::vector<std::string> &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                 uint32_t inHeightImage, uint32_t inHeightBlankSpace, std::vector<char> &inBitmap)
        :heightImage(inHeightImage), heightBlankSpace(inHeightBlankSpace), width(inWidth),
         heightTexture(inHeightTexture), bitmap(inBitmap), textureImages()
    {
        for (uint32_t i=0; i < symbols.size(); i++) {
            TextureImage tex = { i };
            textureImages.insert(std::make_pair(symbols[i], tex));
        }
    }
};
#endif
