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

#ifndef RAINBOWDICE_TEXTUREATLASVULKAN_H
#define RAINBOWDICE_TEXTUREATLASVULKAN_H

#include "vulkanWrapper.hpp"
#include "text.hpp"
#include "rainbowDiceGlobal.hpp"
#include "graphicsVulkan.hpp"

class TextureVulkan {
    std::shared_ptr<TextureAtlas> m_textureAtlas;
    std::shared_ptr<vulkan::ImageSampler> m_textureSampler;

public:
    TextureVulkan(std::shared_ptr<TextureAtlas> textureAtlas,
            std::shared_ptr<vulkan::ImageSampler> inSampler)
            : m_textureAtlas{std::move(textureAtlas)}, m_textureSampler{std::move(inSampler)} {
    }

    TextureVulkan(std::shared_ptr<vulkan::ImageSampler> inSampler,
                  std::vector<std::string> const &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                  std::vector<std::pair<float, float>> const &textureCoordsLeftRight,
                  std::vector<std::pair<float, float>> const &textureCoordsTopBottom,
                  std::unique_ptr<unsigned char[]> &&inBitmap, uint32_t inBitmapLength)
        : m_textureAtlas{new TextureAtlas(symbols, inWidth, inHeightTexture,
                textureCoordsLeftRight, textureCoordsTopBottom,
                std::move(inBitmap), inBitmapLength)},
          m_textureSampler{std::move(inSampler)}
    {}

    std::vector<VkDescriptorImageInfo> getImageInfosForDescriptorSet() {
        std::vector<VkDescriptorImageInfo> imageInfos;
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureSampler->imageView()->imageView().get();
        imageInfo.sampler = m_textureSampler->sampler().get();
        imageInfos.push_back(imageInfo);
        return imageInfos;
    }

    inline std::shared_ptr<TextureAtlas> const &textureAtlas() {
        return m_textureAtlas;
    }
};

#endif //RAINBOWDICE_TEXTUREATLASVULKAN_H
