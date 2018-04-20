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
#include <vulkan/vulkan.h>

#include <vector>
/* texture image */
struct TextureImage {
    uint32_t width;
    uint32_t height;
    void *bitmap;
    bool hasTexture;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
};

class TextureAtlas {
private:
    std::map<std::string, TextureImage> textureImages;

public:
    TextureAtlas() {}
    void destroy();

    void addAtlasSymbols(std::vector<std::string> &symbols);
    void addTextureImage(std::string symbol, TextureImage &image);
    void addSymbol(std::string &symbol, uint32_t width, uint32_t height, void *bitmap);
    bool hasSymbol(std::string symbol);
    TextureImage getImage(std::string symbol);
    size_t nbrSymbols();
    uint32_t getArrayIndex(std::string symbol);
    std::vector<VkDescriptorImageInfo> getImageInfosForDescriptorSet();
};
#endif
