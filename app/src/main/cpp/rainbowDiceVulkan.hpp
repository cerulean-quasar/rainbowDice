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

    void loadModel(std::shared_ptr<TextureAtlas> const &texAtlas) {
        die->loadModel(texAtlas);
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

    void destroyVulkanMemory() {
        uniformBuffer.reset();
        indexBuffer.reset();
        vertexBuffer.reset();
        descriptorSet.reset();
        m_device.reset();
    }

    void updateUniformBuffer(glm::mat4 const &proj, glm::mat4 const &view) {
        UniformBufferObject ubo;
        ubo.proj = die->alterPerspective(proj);
        ubo.view = view;
        ubo.model = die->model();
        uniformBuffer->copyRawTo(&ubo, sizeof (ubo));
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
        die = std::move(createDice(symbols, color));
    }
};

class RainbowDiceVulkan : public RainbowDice {
public:
    explicit RainbowDiceVulkan(std::shared_ptr<WindowType> window)
            : RainbowDice{},
              m_instance{new vulkan::Instance{std::move(window)}},
              m_device{new vulkan::Device{m_instance}},
              m_swapChain{new vulkan::SwapChain{m_device}},
              m_renderPass{new vulkan::RenderPass{m_device, m_swapChain}},
              m_descriptorSetLayout{new DiceDescriptorSetLayout{m_device}},
              m_descriptorPools{new vulkan::DescriptorPools{m_device, m_descriptorSetLayout}},
              m_graphicsPipeline{new vulkan::Pipeline{m_swapChain, m_renderPass, m_descriptorSetLayout,
                  getBindingDescription(), getAttributeDescriptions(), SHADER_VERT_FILE, SHADER_FRAG_FILE}},
              m_commandPool{new vulkan::CommandPool{m_device}},
              m_viewPointBuffer{vulkan::Buffer::createUniformBuffer(m_device, sizeof (glm::vec3))},
              m_imageAvailableSemaphore{new vulkan::Semaphore{m_device}},
              m_renderFinishedSemaphore{new vulkan::Semaphore{m_device}},
              m_depthImageView{new vulkan::ImageView{vulkan::ImageFactory::createDepthImage(m_swapChain),
                                                     m_device->depthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT}},
              m_swapChainCommands{new vulkan::SwapChainCommands{m_swapChain, m_commandPool,
                                                                m_renderPass, m_depthImageView}},
              m_preTransform{1.0f}
    {
        setView();
        updatePerspectiveMatrix(m_swapChain->extent().width, m_swapChain->extent().height);

        // copy the view point of the scene into device memory to send to the fragment shader for the
        // Blinn-Phong lighting model.  Copy it over here too since it is a constant.
        m_viewPointBuffer->copyRawTo(&m_viewPoint, sizeof(m_viewPoint));


        switch (m_swapChain->preTransform()) {
            case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
                m_preTransform = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
                m_preTransform = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
                m_preTransform = glm::rotate(glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
                m_preTransform[0][0] = -1.0f;
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
                m_preTransform[0][0] = -1.0f;
                m_preTransform = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * m_preTransform;
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
                m_preTransform[0][0] = -1.0f;
                m_preTransform = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * m_preTransform;
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
                m_preTransform[0][0] = -1.0f;
                m_preTransform = glm::rotate(glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * m_preTransform;
                break;
            case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
            default:
                break;
        }
    }

    void initModels() override;

    void initThread() override {}

    void cleanupThread() override {}

    void drawFrame() override;

    bool updateUniformBuffer() override;

    bool allStopped() override;

    std::vector<std::vector<uint32_t>> getDiceResults() override;

    void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &rerollIndices,
                            std::vector<float> const &color) override;

    void recreateModels() override;

    void recreateSwapChain(uint32_t width, uint32_t height) override;

    void updateAcceleration(float x, float y, float z) override;

    void resetPositions() override;

    void resetPositions(std::set<int> &dice) override;

    void resetToStoppedPositions(std::vector<uint32_t> const &symbols) override;

    void addRollingDice() override;

    bool needsReroll() override;

    void newSurface(std::shared_ptr<WindowType> surface) override;

    void updatePerspectiveMatrix(uint32_t surfaceWidth, uint32_t surfaceHeight) override {
        RainbowDice::updatePerspectiveMatrix(surfaceWidth, surfaceHeight);

        /* GLM has the y axis inverted from Vulkan's perspective, invert the y-axis on the
         * projection matrix.
         */
        m_proj[1][1] *= -1;

        /* some hardware have their screen rotated in different directions.  We need to apply the
         * preTransform that we promised the window manager/hardware that we would do.
         */
        m_proj = m_preTransform * m_proj;
    }

    void scale(float scaleFactor) override {
        RainbowDice::scale(scaleFactor);

        m_viewPointBuffer->copyRawTo(&m_viewPoint, sizeof(m_viewPoint));
        for (auto const & die : dice) {
            die->updateUniformBuffer(m_proj, m_view);
        }
    }

    void scroll(float distanceX, float distanceY) override {
        auto ext = m_swapChain->extent();
        RainbowDice::scroll(distanceX, distanceY, ext.width, ext.height);

        m_viewPointBuffer->copyRawTo(&m_viewPoint, sizeof(m_viewPoint));
        for (auto const & die : dice) {
            die->updateUniformBuffer(m_proj, m_view);
        }
    }

    void setTexture(std::shared_ptr<TextureAtlas> texture) override {
        std::shared_ptr<vulkan::ImageView> imgView = std::make_shared<vulkan::ImageView>(
                createTextureImage(texture->getImageWidth(), texture->getImageHeight(),
                                   texture->bitmap(), texture->bitmapLength()),
                VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        std::shared_ptr<vulkan::ImageSampler> imgSampler = std::make_shared<vulkan::ImageSampler>(
                m_device, m_commandPool, imgView);
        m_texture = std::make_shared<TextureVulkan>(std::move(texture), imgSampler);
    }

    void setDice(std::string const &inDiceName,
            std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions) override {
        RainbowDice::setDice(inDiceName, inDiceDescriptions);

        dice.clear();
        for (auto const &diceDescription : m_diceDescriptions) {
            for (int i = 0; i < diceDescription->m_nbrDice; i++) {
                loadObject(diceDescription->m_symbols, diceDescription->m_rerollOnIndices,
                           diceDescription->m_color);
            }
        }
    }

    ~RainbowDiceVulkan() override = default;
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
    std::shared_ptr<vulkan::Semaphore> m_imageAvailableSemaphore;
    std::shared_ptr<vulkan::Semaphore> m_renderFinishedSemaphore;

    /* depth buffer image */
    std::shared_ptr<vulkan::ImageView> m_depthImageView;

    std::shared_ptr<vulkan::SwapChainCommands> m_swapChainCommands;

    std::shared_ptr<TextureVulkan> m_texture;

    /* the preTransform matrix.  Perform this transform after all other matrices have been applied.
     * It matches what we would promise the hardware we would do.
     */
    glm::mat4 m_preTransform;

    std::shared_ptr<vulkan::Buffer> createVertexBuffer(Dice *die);
    std::shared_ptr<vulkan::Buffer> createIndexBuffer(Dice *die);

    void cleanupSwapChain();
    void initializeCommandBuffers();
    void updateDepthResources();
    std::shared_ptr<vulkan::Image> createTextureImage(uint32_t texWidth, uint32_t texHeight,
                                                      std::unique_ptr<unsigned char[]> const &bitmap,
                                                      size_t bitmapSize);
};
#endif
