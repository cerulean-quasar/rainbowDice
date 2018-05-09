//
// Created by cerulean.quasar on 5/5/18.
//

#ifndef RAINBOWDICE_TEXTUREATLASVULKAN_H
#define RAINBOWDICE_TEXTUREATLASVULKAN_H

#include "vulkanWrapper.hpp"
#include "text.hpp"
#include "rainbowDiceGlobal.hpp"

class TextureAtlasVulkan : public TextureAtlas {
    bool hasTexture;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

public:
    TextureAtlasVulkan(std::vector<std::string> &symbols, uint32_t inWidth, uint32_t inHeightTexture, uint32_t inHeightImage,  std::vector<char> &inBitmap)
        : TextureAtlas(symbols, inWidth, inHeightTexture, inHeightImage, inBitmap), hasTexture(false){}

    bool hasVulkanTexture(std::string symbol) {return hasTexture;}
    void destroy(VkDevice logicalDevice) {
        if (hasTexture) {
            vkDestroySampler(logicalDevice, textureSampler, nullptr);
            vkDestroyImageView(logicalDevice, textureImageView, nullptr);
            vkDestroyImage(logicalDevice, textureImage, nullptr);
            vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
        }

        hasTexture = false;
    }
    void addTextureImage(VkImage inImage, VkDeviceMemory inImageMemory, VkImageView inImageView, VkSampler inSampler) {
        if (!hasTexture) {
            textureImage = inImage;
            textureImageMemory = inImageMemory;
            textureImageView = inImageView;
            textureSampler = inSampler;
            hasTexture = true;
        } else {
            /* should not happen */
            throw std::runtime_error(std::string("Already have the texture image Vulkan structures."));
        }
    }

    std::vector<VkDescriptorImageInfo> getImageInfosForDescriptorSet() {
        std::vector<VkDescriptorImageInfo> imageInfos;
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;
        imageInfos.push_back(imageInfo);
        return imageInfos;
    }
};


#endif //RAINBOWDICE_TEXTUREATLASVULKAN_H
