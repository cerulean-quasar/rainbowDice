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
#include <stdexcept>
#include <cstdlib>

#include "text.hpp"
#include "rainbowDiceGlobal.hpp"

void TextureAtlas::destroy() {
    for (auto it = textureImages.begin(); it != textureImages.end(); it++) {
        vkDestroySampler(logicalDevice, it->second.textureSampler, nullptr);
        vkDestroyImageView(logicalDevice, it->second.textureImageView, nullptr);
        vkDestroyImage(logicalDevice, it->second.textureImage, nullptr);
        vkFreeMemory(logicalDevice, it->second.textureImageMemory, nullptr);
        if (it->second.bitmap != nullptr) {
            free(it->second.bitmap);
        }
    }
    textureImages.clear();
}

void TextureAtlas::addAtlasSymbols(std::vector<std::string> &symbols) {
    for (auto symbol : symbols) {
        TextureImage image = {};
        textureImages.insert(std::pair<std::string, TextureImage>(symbol, image));
    }
}

void TextureAtlas::addSymbol(std::string &symbol, uint32_t width, uint32_t height, void *bitmap) {
    TextureImage image = {};
    image.width = width;
    image.height = height;
    image.bitmap = bitmap;
    textureImages.insert(std::pair<std::string, TextureImage>(symbol, image));
}

void TextureAtlas::addTextureImage(std::string symbol, TextureImage &image) {
    std::map<std::string, TextureImage>::iterator it = textureImages.find(symbol);
    if (it != textureImages.end()) {
        if (!it->second.hasTexture) {
            // TODO: get rid of the ick
            it->second.textureImage = image.textureImage;
            it->second.textureImageMemory = image.textureImageMemory;
            it->second.textureImageView = image.textureImageView;
            it->second.textureSampler = image.textureSampler;
            it->second.hasTexture = true;
        } else {
            /* should not happen */
            throw std::runtime_error(std::string("Already have an image for symbol: ") + symbol);
        }
    }
    textureImages.insert(std::pair<std::string, TextureImage>(symbol, image));
}

bool TextureAtlas::hasSymbol(std::string symbol) {
    auto it = textureImages.find(symbol);

    if (it == textureImages.end()) {
        return false;
    } else if (it->second.hasTexture == false) {
        // TODO: get rid of the ick
        return false;
    } else {
        return true;
    }
}

TextureImage TextureAtlas::getImage(std::string symbol) {
    std::map<std::string, TextureImage>::iterator it = textureImages.find(symbol);
    if (it == textureImages.end()) {
        /* should not happen */
        throw std::runtime_error(std::string("Texture image not found for: ") + symbol);
    }
    return it->second;
}

size_t TextureAtlas::nbrSymbols() {
    return textureImages.size();
}

uint32_t TextureAtlas::getArrayIndex(std::string symbol) {
    int i=0;
    for (auto it = textureImages.begin(); it != textureImages.end(); it++) {
        if (it->first == symbol) {
            return i;
        }
        i++;
    }
    /* should not happen */
    throw std::runtime_error(std::string("Texture image not found for: ") + symbol);
}

std::vector<VkDescriptorImageInfo> TextureAtlas::getImageInfosForDescriptorSet() {
    std::vector<VkDescriptorImageInfo> imageInfos = {};
    for (auto it = textureImages.begin(); it != textureImages.end(); it++) {
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = it->second.textureImageView;
        imageInfo.sampler = it->second.textureSampler;
        imageInfos.push_back(imageInfo);
    }
    return imageInfos;
}
