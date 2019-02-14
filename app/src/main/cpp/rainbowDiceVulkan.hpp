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
#ifndef RAINBOWDICE_VULKAN_HPP
#define RAINBOWDICE_VULKAN_HPP
#include "vulkanWrapper.hpp"

#include <stdexcept>
#include <functional>
#include <vector>
#include <string>
#include <string.h>
#include <set>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <list>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "rainbowDiceGlobal.hpp"
#include "rainbowDice.hpp"
#include "text.hpp"
#include "dice.hpp"
#include "graphicsVulkan.hpp"
#include "TextureAtlasVulkan.h"

VkVertexInputBindingDescription getBindingDescription();
std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

class DiceDescriptorSetLayout : public vulkan::DescriptorSetLayout {
public:
    DiceDescriptorSetLayout(std::shared_ptr<vulkan::Device> const &inDevice)
            : m_device{inDevice},
              m_descriptorSetLayout{},
              m_poolInfo{},
              m_poolSizes{}
    {
        m_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        m_poolSizes[0].descriptorCount = m_numberOfDescriptorSetsInPool;
        m_poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        m_poolSizes[1].descriptorCount = m_numberOfDescriptorSetsInPool;
        m_poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        m_poolSizes[2].descriptorCount = m_numberOfDescriptorSetsInPool;
        m_poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        m_poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
        m_poolInfo.pPoolSizes = m_poolSizes.data();
        m_poolInfo.maxSets = m_numberOfDescriptorSetsInPool;

        createDescriptorSetLayout();
    }

    void updateDescriptorSet(std::shared_ptr<vulkan::Buffer> const &uniformBuffer,
                             std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
                             std::shared_ptr<vulkan::DescriptorSet> const &descriptorSet,
                             std::vector<VkDescriptorImageInfo> const &imageInfos);

    virtual std::shared_ptr<VkDescriptorSetLayout_T> const &descriptorSetLayout() {
        return m_descriptorSetLayout;
    }

    virtual uint32_t numberOfDescriptors() { return m_numberOfDescriptorSetsInPool; }

    virtual VkDescriptorPoolCreateInfo const &poolCreateInfo() {
        return m_poolInfo;
    }
private:
    static uint32_t constexpr m_numberOfDescriptorSetsInPool = 1024;
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<VkDescriptorSetLayout_T> m_descriptorSetLayout;
    VkDescriptorPoolCreateInfo m_poolInfo;
    std::array<VkDescriptorPoolSize, 3> m_poolSizes;

    void createDescriptorSetLayout();
};

struct Dice {
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<DicePhysicsModel> die;

    /* vertex buffer and index buffer. the index buffer indicates which vertices to draw and in
     * the specified order.  Note, vertices can be listed twice if they should be part of more
     * than one triangle.
     */
    std::shared_ptr<vulkan::Buffer> vertexBuffer;
    std::shared_ptr<vulkan::Buffer> indexBuffer;

    /* for passing data other than the vertex data to the vertex shader */
    std::shared_ptr<vulkan::DescriptorSet> descriptorSet;
    std::shared_ptr<vulkan::Buffer> uniformBuffer;

    bool isBeingReRolled;
    std::vector<uint32_t> rerollIndices;

    Dice(std::shared_ptr<vulkan::Device> const &inDevice,
         std::vector<std::string> const &symbols, std::vector<uint32_t> const &inRerollIndices,
         glm::vec3 &position, std::vector<float> const &color)
            : m_device{inDevice},
              die(nullptr),
              rerollIndices{inRerollIndices}
    {
        initDice(symbols, position, color);
    }

    void loadModel(int width, int height, std::shared_ptr<TextureAtlas> const &texAtlas) {
        die->loadModel(texAtlas);
        die->setView();
        die->updatePerspectiveMatrix(width, height);
    }

    void init(std::shared_ptr<vulkan::DescriptorSet> inDescriptorSet,
              std::shared_ptr<vulkan::Buffer> inIndexBuffer,
              std::shared_ptr<vulkan::Buffer> inVertexBuffer,
              std::shared_ptr<vulkan::Buffer> inUniformBuffer) {
        uniformBuffer = inUniformBuffer;
        indexBuffer = inIndexBuffer;
        vertexBuffer = inVertexBuffer;
        descriptorSet = inDescriptorSet;
    }

    void updateUniformBuffer() {
        uniformBuffer->copyRawTo(&die->ubo, sizeof(die->ubo));
    }

    bool needsReroll() {
        if (!die->isStopped()) {
            // should not happen!
            return false;
        }

        if (isBeingReRolled) {
            return false;
        }

        uint32_t result = die->getResult();

        for (auto const & rerollIndex : rerollIndices) {
            if (result % die->getNumberOfSymbols() == rerollIndex) {
                return true;
            }
        }

        return false;
    }

private:
    void initDice(std::vector<std::string> const &symbols, glm::vec3 &position, std::vector<float> const &color) {
        isBeingReRolled = false;
        long nbrSides = symbols.size();
        if (nbrSides == 2) {
            die.reset(new DiceModelCoin(symbols, position, color));
        } else if (nbrSides == 4) {
            die.reset(new DiceModelTetrahedron(symbols, position, color));
        } else if (nbrSides == 12) {
            die.reset(new DiceModelDodecahedron(symbols, position, color));
        } else if (nbrSides == 20) {
            die.reset(new DiceModelIcosahedron(symbols, position, color));
        } else if (nbrSides == 30) {
            die.reset(new DiceModelRhombicTriacontahedron(symbols, position, color));
        } else if (6 % nbrSides == 0) {
            die.reset(new DiceModelCube(symbols, position, color));
        } else {
            die.reset(new DiceModelHedron(symbols, position, color));
        }
    }
};

class RainbowDiceVulkan : public RainbowDice {
public:
    RainbowDiceVulkan(WindowType *window, std::vector<std::string> &symbols, uint32_t inWidth, uint32_t inHeightTexture,
                      std::vector<std::pair<float, float>> const &textureCoordsLeftRight,
                      std::vector<std::pair<float, float>> const &textureCoordsTopBottom,
                      std::unique_ptr<unsigned char[]> const &inBitmap, size_t inBitmapSize)
            : m_instance{new vulkan::Instance{window}},
              m_device{new vulkan::Device{m_instance}},
              m_swapChain{new vulkan::SwapChain{m_device}},
              m_renderPass{new vulkan::RenderPass{m_device, m_swapChain}},
              m_descriptorSetLayout{new DiceDescriptorSetLayout{m_device}},
              m_descriptorPools{new vulkan::DescriptorPools{m_device, m_descriptorSetLayout}},
              m_graphicsPipeline{new vulkan::Pipeline{m_swapChain, m_renderPass, m_descriptorSetLayout,
                  getBindingDescription(), getAttributeDescriptions(), SHADER_VERT_FILE, SHADER_FRAG_FILE}},
              m_commandPool{new vulkan::CommandPool{m_device}},
              m_viewPointBuffer{vulkan::Buffer::createUniformBuffer(m_device, sizeof (glm::vec3))},
              m_imageAvailableSemaphore{m_device},
              m_renderFinishedSemaphore{m_device},
              m_depthImageView{new vulkan::ImageView{vulkan::ImageFactory::createDepthImage(m_swapChain),
                                                     m_device->depthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT}},
              m_swapChainCommands{new vulkan::SwapChainCommands{m_swapChain, m_commandPool,
                                                                m_renderPass, m_depthImageView}},
              m_textureAtlas{new TextureAtlasVulkan{std::shared_ptr<vulkan::ImageSampler>{
                  new vulkan::ImageSampler{m_device, m_commandPool,
                        std::shared_ptr<vulkan::ImageView>{new vulkan::ImageView{
                            createTextureImage(inWidth, inHeightTexture, inBitmap, inBitmapSize), VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_ASPECT_COLOR_BIT}}}},
                  symbols, inWidth, inHeightTexture, textureCoordsLeftRight, textureCoordsTopBottom}}
    {
        // copy the view point of the scene into device memory to send to the fragment shader for the
        // Blinn-Phong lighting model.  Copy it over here too since it is a constant.
        m_viewPointBuffer->copyRawTo(&viewPoint, sizeof(viewPoint));
    }

    virtual void initModels();

    virtual void initThread() {}

    virtual void cleanupThread() {}

    virtual void drawFrame();

    virtual bool updateUniformBuffer();

    virtual bool allStopped();

    virtual std::vector<std::vector<uint32_t>> getDiceResults();

    virtual void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &rerollIndices,
                            std::vector<float> const &color);

    virtual void recreateModels();

    virtual void recreateSwapChain();

    virtual void updateAcceleration(float x, float y, float z);

    virtual void resetPositions();

    virtual void resetPositions(std::set<int> &dice);

    virtual void resetToStoppedPositions(std::vector<uint32_t> const &symbols);

    virtual void addRollingDice();

    virtual bool needsReroll();

    virtual ~RainbowDiceVulkan() { }
private:
    static std::string const SHADER_VERT_FILE;
    static std::string const SHADER_FRAG_FILE;

    std::shared_ptr<vulkan::Instance> m_instance;
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<vulkan::SwapChain> m_swapChain;
    std::shared_ptr<vulkan::RenderPass> m_renderPass;

    /* for passing data other than the vertex data to the vertex shader */
    std::shared_ptr<DiceDescriptorSetLayout> m_descriptorSetLayout;
    std::shared_ptr<vulkan::DescriptorPools> m_descriptorPools;

    std::shared_ptr<vulkan::Pipeline> m_graphicsPipeline;
    std::shared_ptr<vulkan::CommandPool> m_commandPool;
    std::shared_ptr<vulkan::Buffer> m_viewPointBuffer;

    typedef std::list<std::shared_ptr<Dice> > DiceList;
    DiceList dice;

    /* use semaphores to coordinate the rendering and presentation. Could also use fences
     * but fences are more for coordinating in our program itself and not for internal
     * Vulkan coordination of resource usage.
     */
    vulkan::Semaphore m_imageAvailableSemaphore;
    vulkan::Semaphore m_renderFinishedSemaphore;

    /* depth buffer image */
    std::shared_ptr<vulkan::ImageView> m_depthImageView;

    std::shared_ptr<vulkan::SwapChainCommands> m_swapChainCommands;

    std::shared_ptr<TextureAtlasVulkan> m_textureAtlas;

    std::shared_ptr<vulkan::Buffer> createVertexBuffer(Dice *die);
    std::shared_ptr<vulkan::Buffer> createIndexBuffer(Dice *die);
    void updatePerspectiveMatrix();

    void cleanupSwapChain();
    void initializeCommandBuffers();
    void updateDepthResources();
    std::shared_ptr<vulkan::Image> createTextureImage(uint32_t texWidth, uint32_t texHeight,
                                                      std::unique_ptr<unsigned char[]> const &bitmap,
                                                      size_t bitmapSize);
};
#endif
