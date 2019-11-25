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

struct PerObjectFragmentVariables {
    int isSelected;
    float edgeWidth;
};

VkVertexInputBindingDescription getBindingDescription();
std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
VkVertexInputBindingDescription getBindingDescriptionOutlineSquare();
std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionsOutlineSquare();

class DiceDescriptorSetLayout : public vulkan::DescriptorSetLayout {
public:
    DiceDescriptorSetLayout(std::shared_ptr<vulkan::Device> inDevice)
            : m_device{std::move(inDevice)},
              m_descriptorSetLayout{},
              m_poolInfo{},
              m_poolSizes{}
    {
        // TODO: do we really need more than one poolSizes entry for VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER?
        // just increase descriptorCount to 3 * m_numberOfDescriptorsSetsInPool
        m_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        m_poolSizes[0].descriptorCount = m_numberOfDescriptorSetsInPool;
        m_poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        m_poolSizes[1].descriptorCount = m_numberOfDescriptorSetsInPool;
        m_poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        m_poolSizes[2].descriptorCount = m_numberOfDescriptorSetsInPool;
        m_poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        m_poolInfo.flags = 0;
        m_poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
        m_poolInfo.pPoolSizes = m_poolSizes.data();
        m_poolInfo.maxSets = m_numberOfDescriptorSetsInPool;
        m_poolInfo.pNext = nullptr;

        createDescriptorSetLayout();
    }

    void updateDescriptorSet(std::shared_ptr<vulkan::Buffer> const &uniformBuffer,
                             std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
                             std::shared_ptr<vulkan::Buffer> const &perObjFragVars,
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

class DiceBoxDescriptorSetLayout : public vulkan::DescriptorSetLayout {
public:
    DiceBoxDescriptorSetLayout(std::shared_ptr<vulkan::Device> inDevice)
            : m_device{std::move(inDevice)},
              m_descriptorSetLayout{},
              m_poolInfo{},
              m_poolSizes{}
    {
        // The MVP matrices and view vector.  There are two descriptors per descriptor set, so
        // multiply m_numberOfDescriptorSetsInPool by 2.
        m_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        m_poolSizes[0].descriptorCount = m_numberOfDescriptorSetsInPool * 2;
        m_poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        m_poolInfo.flags = 0;
        m_poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
        m_poolInfo.pPoolSizes = m_poolSizes.data();
        m_poolInfo.maxSets = m_numberOfDescriptorSetsInPool * 2;
        m_poolInfo.pNext = nullptr;

        createDescriptorSetLayout();
    }

    void updateDescriptorSet(std::shared_ptr<vulkan::Buffer> const &uniformBuffer,
                             std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
                             std::shared_ptr<vulkan::DescriptorSet> const &descriptorSet);

    virtual std::shared_ptr<VkDescriptorSetLayout_T> const &descriptorSetLayout() {
        return m_descriptorSetLayout;
    }

    virtual uint32_t numberOfDescriptors() { return m_numberOfDescriptorSetsInPool; }

    virtual VkDescriptorPoolCreateInfo const &poolCreateInfo() {
        return m_poolInfo;
    }
private:
    static uint32_t constexpr m_numberOfDescriptorSetsInPool = 1;
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<VkDescriptorSetLayout_T> m_descriptorSetLayout;
    VkDescriptorPoolCreateInfo m_poolInfo;
    std::array<VkDescriptorPoolSize, 1> m_poolSizes;

    void createDescriptorSetLayout();
};

struct VulkanGraphics {
    using Buffer = std::shared_ptr<vulkan::Buffer>;
};

class DiceBoxVulkan : public DiceBox<VulkanGraphics> {
public:
    inline auto const &descriptorSet() { return m_descriptorSet; }

    void updateUniformBuffer(glm::mat4 const &proj, glm::mat4 const &view) {
        UniformBufferObject ubo;
        ubo.proj = proj;
        ubo.view = view;
        ubo.model = glm::mat4(1.0f);
        m_uniformBuffer->copyRawTo(&ubo, sizeof(ubo));
    }

    void updateMaxXYZ(float maxX, float maxY, float maxZ) override {
        populateVerticesIndices(maxX, maxY, maxZ);

        vulkan::updateArrayBuffer(m_device, m_commandPool, m_verticesDiceBox, m_vertexBuffer);
    }

    DiceBoxVulkan(std::shared_ptr<vulkan::Device> inDevice,
               std::shared_ptr<DiceBoxDescriptorSetLayout> const &descriptorSetLayout,
               std::shared_ptr<vulkan::DescriptorPools> const &descriptorPools,
               std::shared_ptr<vulkan::CommandPool> commandPool,
               std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
               glm::mat4 const &proj,
               glm::mat4 const &view,
               float maxX,
               float maxY,
               float maxZ)
            : DiceBox<VulkanGraphics>{maxX, maxY, maxZ},
              m_device{std::move(inDevice)},
              m_descriptorSet{descriptorPools->allocateDescriptor()},
              m_uniformBuffer{vulkan::Buffer::createUniformBuffer(
                      m_device, sizeof (UniformBufferObject))},
              m_commandPool{std::move(commandPool)}
    {
        descriptorSetLayout->updateDescriptorSet(m_uniformBuffer, viewPointBuffer, m_descriptorSet);
        m_vertexBuffer = createVertexBuffer(m_commandPool, m_verticesDiceBox);
        m_indexBuffer = createIndexBuffer(m_commandPool, m_indicesDiceBox);
        updateUniformBuffer(proj, view);
    }

    ~DiceBoxVulkan() override = default;
private:
    std::shared_ptr<vulkan::Device> m_device;

    /* for passing data other than the vertex data to the vertex shader */
    std::shared_ptr<vulkan::DescriptorSet> m_descriptorSet;
    std::shared_ptr<vulkan::Buffer> m_uniformBuffer;
    std::shared_ptr<vulkan::CommandPool> m_commandPool;

    std::shared_ptr<vulkan::Buffer> createVertexBuffer(
            std::shared_ptr<vulkan::CommandPool> const &commandPool,
            std::vector<VertexSquareOutline> const &vertices) {
        return vulkan::createArrayBuffer(m_device, commandPool, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    std::shared_ptr<vulkan::Buffer> createIndexBuffer(
            std::shared_ptr<vulkan::CommandPool> const &commandPool,
            std::vector<uint32_t> const &indices) {
        return vulkan::createArrayBuffer(m_device, commandPool, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
};

class DiceVulkan : public DiceGraphics<VulkanGraphics> {
public:
    inline auto const &descriptorSet() { return m_descriptorSet; }

    void updateUniformBufferVertexVariables(glm::mat4 const &proj, glm::mat4 const &view) {
        UniformBufferObject ubo;
        ubo.proj = m_die->alterPerspective(proj);
        ubo.view = view;
        ubo.model = m_die->model();
        m_uniformBuffer->copyRawTo(&ubo, sizeof(ubo));
    }

    void updateUniformBuffer(glm::mat4 const &proj, glm::mat4 const &view) {
        updateUniformBufferVertexVariables(proj, view);
        if (m_die->isStopped()) {
            updateUniformBufferFragmentVariables();
        }
    }

    void updateUniformBufferFragmentVariables() {
        PerObjectFragmentVariables fragmentVariables = {};
        fragmentVariables.isSelected = m_isSelected ? 1 : 0;
        fragmentVariables.edgeWidth = m_die->edgeWidth();
        m_uniformBufferFrag->copyRawTo(&fragmentVariables, sizeof (fragmentVariables));
    }

    void toggleSelected() {
        m_isSelected = !m_isSelected;
        updateUniformBufferFragmentVariables();
    }

    DiceVulkan(std::shared_ptr<vulkan::Device> inDevice,
               std::shared_ptr<TextureVulkan> const &texture,
               std::shared_ptr<DiceDescriptorSetLayout> const &descriptorSetLayout,
               std::shared_ptr<vulkan::DescriptorPools> const &descriptorPools,
               std::shared_ptr<vulkan::CommandPool> const &commandPool,
               std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
               glm::mat4 const &proj,
               glm::mat4 const &view,
               std::vector<std::string> const &symbols,
               std::vector<uint32_t> const &inRerollIndices,
               std::vector<float> const &color)
            : DiceGraphics{symbols, inRerollIndices, color, texture->textureAtlas()},
              m_device{std::move(inDevice)},
              m_descriptorSet{descriptorPools->allocateDescriptor()},
              m_uniformBuffer{vulkan::Buffer::createUniformBuffer(
                      m_device, sizeof (UniformBufferObject))},
              m_uniformBufferFrag{vulkan::Buffer::createUniformBuffer(
                      m_device, sizeof (PerObjectFragmentVariables))}
    {
        descriptorSetLayout->updateDescriptorSet(m_uniformBuffer, viewPointBuffer,
                                                 m_uniformBufferFrag, m_descriptorSet,
                                                 texture->getImageInfosForDescriptorSet());
        m_vertexBuffer = createVertexBuffer(commandPool, m_die->getVertices());
        m_indexBuffer = createIndexBuffer(commandPool, m_die->getIndices());
        updateUniformBuffer(proj, view);
        updateUniformBufferFragmentVariables();
    }

    ~DiceVulkan() override = default;
private:
    std::shared_ptr<vulkan::Device> m_device;

    /* for passing data other than the vertex data to the vertex shader */
    std::shared_ptr<vulkan::DescriptorSet> m_descriptorSet;
    std::shared_ptr<vulkan::Buffer> m_uniformBuffer;
    std::shared_ptr<vulkan::Buffer> m_uniformBufferFrag;

    std::shared_ptr<vulkan::Buffer> createVertexBuffer(
            std::shared_ptr<vulkan::CommandPool> const &commandPool,
            std::vector<Vertex> const &vertices) {
        return vulkan::createArrayBuffer(m_device, commandPool, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    std::shared_ptr<vulkan::Buffer> createIndexBuffer(
            std::shared_ptr<vulkan::CommandPool> const &commandPool,
            std::vector<uint32_t> const &indices) {
        return vulkan::createArrayBuffer(m_device, commandPool, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
};

class RainbowDiceVulkan : public RainbowDiceGraphics<DiceVulkan, DiceBoxVulkan> {
public:
    explicit RainbowDiceVulkan(
            std::shared_ptr<WindowType> window,
            bool inDrawRollingDice,
            bool reverseGravity)
            : RainbowDiceGraphics{inDrawRollingDice, reverseGravity},
              m_instance{new vulkan::Instance{std::move(window)}},
              m_device{new vulkan::Device{m_instance}},
              m_swapChain{new vulkan::SwapChain{m_device}},
              m_renderPass{new vulkan::RenderPass{m_device, m_swapChain}},
              m_descriptorSetLayout{new DiceDescriptorSetLayout{m_device}},
              m_descriptorSetLayoutDiceBox{},
              m_descriptorPools{new vulkan::DescriptorPools{m_device, m_descriptorSetLayout}},
              m_descriptorPoolsDiceBox{},
              m_graphicsPipeline{new vulkan::Pipeline{m_swapChain, m_renderPass, m_descriptorSetLayout,
                                                      std::shared_ptr<vulkan::Pipeline>(),
                                                      getBindingDescription(), getAttributeDescriptions(), SHADER_VERT_FILE, SHADER_FRAG_FILE}},
              m_graphicsPipelineDiceBox{},
              m_commandPool{new vulkan::CommandPool{m_device}},
              m_viewPointBuffer{vulkan::Buffer::createUniformBuffer(m_device, sizeof (glm::vec3))},
              m_imageAvailableSemaphore{new vulkan::Semaphore{m_device}},
              m_renderFinishedSemaphore{new vulkan::Semaphore{m_device}},
              m_depthImageView{new vulkan::ImageView{vulkan::ImageFactory::createDepthImage(m_swapChain),
                                                     m_device->depthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT}},
              m_swapChainCommands{new vulkan::SwapChainCommands{m_swapChain, m_commandPool,
                                                                m_renderPass, m_depthImageView}},
              m_projWithPreTransform{},
              m_width{m_swapChain->extent().width},
              m_height{m_swapChain->extent().height}
    {
        setView();
        updatePerspectiveMatrix(m_swapChain->extent().width, m_swapChain->extent().height);

        // copy the view point of the scene into device memory to send to the fragment shader for the
        // Blinn-Phong lighting model.  Copy it over here too.
        m_viewPointBuffer->copyRawTo(&m_viewPoint, sizeof(m_viewPoint));

        // must happen after view and projection matrix and view point buffer initialization
        if (!reverseGravity) {
            m_descriptorSetLayoutDiceBox = std::make_shared<DiceBoxDescriptorSetLayout>(m_device);

            // Use separate descriptor pool here because descriptor sets are reused and if a
            // descriptor set with a different layout (i.e. the dice box descriptor set) is reused
            // for a dice descriptor set, bad things will happen.
            m_descriptorPoolsDiceBox = std::make_shared<vulkan::DescriptorPools>(m_device, m_descriptorSetLayoutDiceBox);
            m_graphicsPipelineDiceBox = std::make_shared<vulkan::Pipeline>(
                    m_swapChain, m_renderPass, m_descriptorSetLayoutDiceBox, m_graphicsPipeline,
                    getBindingDescriptionOutlineSquare(), getAttributeDescriptionsOutlineSquare(),
                    SHADER_LINES_VERT_FILE, SHADER_LINES_FRAG_FILE);
            m_diceBox = std::make_shared<DiceBoxVulkan>(m_device, m_descriptorSetLayoutDiceBox, m_descriptorPoolsDiceBox,
                    m_commandPool, m_viewPointBuffer, m_proj, m_view,
                    m_screenWidth/2.0f, m_screenHeight/2.0f, M_maxDicePosZ);
        }
    }

    void initModels() override;

    void initThread() override {}

    void cleanupThread() override {}

    void drawFrame() override;

    bool updateUniformBuffer() override;

    void recreateSwapChain(uint32_t width, uint32_t height) override;

    void resetToStoppedPositions(std::vector<std::vector<uint32_t>> const &symbols) override;

    bool changeDice(std::string const &inDiceName,
                    std::vector<std::shared_ptr<DiceDescription>> const &inDiceDescriptions,
                    std::shared_ptr<TextureAtlas> inTexture) override {
        bool hasResult = RainbowDiceGraphics<DiceVulkan, DiceBoxVulkan>::changeDice(inDiceName, inDiceDescriptions, inTexture);
        for (auto const &dice : m_dice) {
            for (auto const &die : dice) {
                die->updateUniformBuffer(m_projWithPreTransform, m_view);
            }
        }
        initializeCommandBuffers();
        return hasResult;
    }

    void addRollingDice() override {
        RainbowDiceGraphics::addRollingDice();
        initializeCommandBuffers();
    }


    GraphicsDescription graphicsDescription() override {
        vulkan::Device::DeviceProperties properties = m_device->properties();
        GraphicsDescription description;
        description.m_isVulkan = true;
        description.m_graphicsName = "Vulkan";
        description.m_deviceName = properties.m_name;
        description.m_version = properties.m_vulkanAPIVersion;
        return std::move(description);
    }

    bool tapDice(float x, float y) override {
        return RainbowDiceGraphics::tapDice(x, y, m_width, m_height);
    }

    bool rerollSelected() override {
        bool hasResult = RainbowDiceGraphics::rerollSelected();
        // in case the number of dice changed.  It could happen if we're not displaying rolling dice
        // and the die being rerolled hits a face that requires a die to be added and rerolled.
        initializeCommandBuffers();
        for (auto const &dice : m_dice) {
            for (auto const &die : dice) {
                die->updateUniformBuffer(m_projWithPreTransform, m_view);
            }
        }
        return hasResult;
    }

    bool addRerollSelected() override {
        bool hasResult = RainbowDiceGraphics::addRerollSelected();
        initializeCommandBuffers();
        for (auto const &dice : m_dice) {
            for (auto const &die : dice) {
                die->updateUniformBuffer(m_projWithPreTransform, m_view);
            }
        }
        return hasResult;
    }

    bool deleteSelected() override {
        bool ret = RainbowDiceGraphics::deleteSelected();
        if (ret) {
            initializeCommandBuffers();
            for (auto const &dice : m_dice) {
                for (auto const &die : dice) {
                    die->updateUniformBuffer(m_projWithPreTransform, m_view);
                }
            }
        }
        return ret;
    }

    void updatePerspectiveMatrix(uint32_t surfaceWidth, uint32_t surfaceHeight) override {
        RainbowDice::updatePerspectiveMatrix(surfaceWidth, surfaceHeight);

        /* GLM has the y axis inverted from Vulkan's perspective, invert the y-axis on the
         * projection matrix.
         */
        m_proj[1][1] *= -1;

        /* some hardware have their screen rotated in different directions.  We need to apply the
         * preTransform that we promised the window manager/hardware that we would do.
         */
        m_projWithPreTransform = preTransform() * m_proj;

        m_width = surfaceWidth;
        m_height = surfaceHeight;
    }

    void scale(float scaleFactor) override {
        RainbowDice::scale(scaleFactor);

        updateViewBuffer();
    }

    void scroll(float distanceX, float distanceY) override {
        auto ext = m_swapChain->extent();
        RainbowDice::scroll(distanceX, distanceY, ext.width, ext.height);

        updateViewBuffer();
    }

    void resetView() override {
        RainbowDice::resetView();

        updateViewBuffer();
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

    ~RainbowDiceVulkan() override = default;
protected:
    std::shared_ptr<DiceVulkan> createDie(std::vector<std::string> const &symbols,
                                      std::vector<uint32_t> const &inRerollIndices,
                                      std::vector<float> const &color) override;
    std::shared_ptr<DiceVulkan> createDie(std::shared_ptr<DiceVulkan> const &inDice) override;
    bool invertY() override { return false; }
private:
    static std::string const SHADER_VERT_FILE;
    static std::string const SHADER_FRAG_FILE;
    static std::string const SHADER_LINES_VERT_FILE;
    static std::string const SHADER_LINES_FRAG_FILE;

    std::shared_ptr<vulkan::Instance> m_instance;
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<vulkan::SwapChain> m_swapChain;
    std::shared_ptr<vulkan::RenderPass> m_renderPass;

    /* for passing data other than the vertex data to the vertex shader */
    std::shared_ptr<DiceDescriptorSetLayout> m_descriptorSetLayout;
    std::shared_ptr<DiceBoxDescriptorSetLayout> m_descriptorSetLayoutDiceBox;
    std::shared_ptr<vulkan::DescriptorPools> m_descriptorPools;
    std::shared_ptr<vulkan::DescriptorPools> m_descriptorPoolsDiceBox;

    std::shared_ptr<vulkan::Pipeline> m_graphicsPipeline;
    std::shared_ptr<vulkan::Pipeline> m_graphicsPipelineDiceBox;
    std::shared_ptr<vulkan::CommandPool> m_commandPool;
    std::shared_ptr<vulkan::Buffer> m_viewPointBuffer;

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

    glm::mat4 m_projWithPreTransform;
    uint32_t m_width;
    uint32_t m_height;

    /* return the preTransform matrix.  Perform this transform after all other matrices have been applied.
     * It matches what we would promise the hardware we would do.
     */
    glm::mat4 preTransform() {
        glm::mat4 preTransformRet{1.0f};

        switch (m_swapChain->preTransform()) {
            case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
                preTransformRet = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
                preTransformRet = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
                preTransformRet = glm::rotate(glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
                preTransformRet[0][0] = -1.0f;
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
                preTransformRet[0][0] = -1.0f;
                preTransformRet = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * preTransformRet;
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
                preTransformRet[0][0] = -1.0f;
                preTransformRet = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * preTransformRet;
                break;
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
                preTransformRet[0][0] = -1.0f;
                preTransformRet = glm::rotate(glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * preTransformRet;
                break;
            case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
            default:
                break;
        }

        return preTransformRet;
    }

    void updateViewBuffer() {
        m_viewPointBuffer->copyRawTo(&m_viewPoint, sizeof(m_viewPoint));
        for (auto const &dice : m_dice) {
            for (auto const &die : dice) {
                die->updateUniformBuffer(m_projWithPreTransform, m_view);
            }
        }

        if (m_viewUpdated) {
            m_viewUpdated = false;
            if (m_diceBox != nullptr) {
                m_diceBox->updateUniformBuffer(m_projWithPreTransform, m_view);
            }
        }
    }

    void cleanupSwapChain();
    void initializeCommandBuffers();
    void updateDepthResources();
    std::shared_ptr<vulkan::Image> createTextureImage(uint32_t texWidth, uint32_t texHeight,
                                                      std::unique_ptr<unsigned char[]> const &bitmap,
                                                      size_t bitmapSize);
};
#endif
