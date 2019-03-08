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
#include <cstdint>
#include <stdexcept>
#include "rainbowDiceGlobal.hpp"
#include "rainbowDiceVulkan.hpp"
#include "TextureAtlasVulkan.h"
#include "android.hpp"

std::string const RainbowDiceVulkan::SHADER_VERT_FILE("shaders/shader.vert.spv");
std::string const RainbowDiceVulkan::SHADER_FRAG_FILE("shaders/shader.frag.spv");

VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);

    /* move to the next data entry after each vertex.  VK_VERTEX_INPUT_RATE_INSTANCE
     * moves to the next data entry after each instance, but we are not using instanced
     * rendering
     */
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.resize(11);

    /* position */
    attributeDescriptions[0].binding = 0; /* binding description to use */
    attributeDescriptions[0].location = 0; /* matches the location in the vertex shader */
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    /* color */
    attributeDescriptions[1].binding = 0; /* binding description to use */
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    /* texture coordinate */
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    /* normal to the surface this vertex is on */
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, normal);

    /* normal to the corner */
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(Vertex, cornerNormal);

    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(Vertex, corner1);


    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(Vertex, corner2);


    attributeDescriptions[7].binding = 0;
    attributeDescriptions[7].location = 7;
    attributeDescriptions[7].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[7].offset = offsetof(Vertex, corner3);


    attributeDescriptions[8].binding = 0;
    attributeDescriptions[8].location = 8;
    attributeDescriptions[8].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[8].offset = offsetof(Vertex, corner4);


    attributeDescriptions[9].binding = 0;
    attributeDescriptions[9].location = 9;
    attributeDescriptions[9].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[9].offset = offsetof(Vertex, corner5);

    attributeDescriptions[10].binding = 0;
    attributeDescriptions[10].location = 10;
    attributeDescriptions[10].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[10].offset = offsetof(Vertex, mode);

    return attributeDescriptions;
}

/* for accessing data other than the vertices from the shaders */
void DiceDescriptorSetLayout::createDescriptorSetLayout() {
    /* MVP matrix */
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;

    /* only accessing the MVP matrix from the vertex shader */
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    /* image sampler */
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    /* View Point vector */
    VkDescriptorSetLayoutBinding viewPointLayoutBinding = {};
    viewPointLayoutBinding.binding = 2;
    viewPointLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    viewPointLayoutBinding.descriptorCount = 1;
    viewPointLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    viewPointLayoutBinding.pImmutableSamplers = nullptr; // Optional

    /* Per object fragment shader data */
    VkDescriptorSetLayoutBinding perObjectFragLayoutBinding = {};
    perObjectFragLayoutBinding.binding = 3;
    perObjectFragLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    perObjectFragLayoutBinding.descriptorCount = 1;
    perObjectFragLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    perObjectFragLayoutBinding.pImmutableSamplers = nullptr; // Optional

    std::array<VkDescriptorSetLayoutBinding, 4> bindings =
            {uboLayoutBinding, samplerLayoutBinding, viewPointLayoutBinding, perObjectFragLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayoutRaw;
    if (vkCreateDescriptorSetLayout(m_device->logicalDevice().get(), &layoutInfo, nullptr, &descriptorSetLayoutRaw) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    auto const &capDevice = m_device;
    auto deleter = [capDevice](VkDescriptorSetLayout descriptorSetLayoutRaw) {
        vkDestroyDescriptorSetLayout(capDevice->logicalDevice().get(), descriptorSetLayoutRaw, nullptr);
    };

    m_descriptorSetLayout.reset(descriptorSetLayoutRaw, deleter);
}

/* descriptor set for the MVP matrix and texture samplers */
void DiceDescriptorSetLayout::updateDescriptorSet(std::shared_ptr<vulkan::Buffer> const &uniformBuffer,
                                                  std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
                                                  std::shared_ptr<vulkan::Buffer> const &perObjFragVars,
                                            std::shared_ptr<vulkan::DescriptorSet> const &descriptorSet,
                                                  std::vector<VkDescriptorImageInfo> const &imageInfos) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer->buffer().get();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet->descriptorSet().get();

    /* must be the same as the binding in the vertex shader */
    descriptorWrites[0].dstBinding = 0;

    /* index into the array of descriptors */
    descriptorWrites[0].dstArrayElement = 0;

    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    /* how many array elements you want to update */
    descriptorWrites[0].descriptorCount = 1;

    /* which one of these pointers needs to be used depends on which descriptorType we are
     * using.  pBufferInfo is for buffer based data, pImageInfo is used for image data, and
     * pTexelBufferView is used for decriptors that refer to buffer views.
     */
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[0].pImageInfo = nullptr; // Optional
    descriptorWrites[0].pTexelBufferView = nullptr; // Optional

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet->descriptorSet().get();
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = imageInfos.size();
    descriptorWrites[1].pImageInfo = imageInfos.data();

    VkDescriptorBufferInfo bufferInfoViewPoint = {};
    bufferInfoViewPoint.buffer = viewPointBuffer->buffer().get();
    bufferInfoViewPoint.offset = 0;
    bufferInfoViewPoint.range = sizeof(glm::vec3);

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSet->descriptorSet().get();
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfoViewPoint;
    descriptorWrites[2].pImageInfo = nullptr; // Optional
    descriptorWrites[2].pTexelBufferView = nullptr; // Optional

    VkDescriptorBufferInfo bufferInfoPerObjFragVars = {};
    bufferInfoPerObjFragVars.buffer = perObjFragVars->buffer().get();
    bufferInfoPerObjFragVars.offset = 0;
    bufferInfoPerObjFragVars.range = sizeof(PerObjectFragmentVariables);

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = descriptorSet->descriptorSet().get();
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &bufferInfoPerObjFragVars;
    descriptorWrites[3].pImageInfo = nullptr; // Optional
    descriptorWrites[3].pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(m_device->logicalDevice().get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void RainbowDiceVulkan::initModels() {
    for (auto && die : dice) {
        die->loadModel(m_texture->textureAtlas());
        std::shared_ptr<vulkan::Buffer> vertexBuffer{createVertexBuffer(die.get())};
        std::shared_ptr<vulkan::Buffer> indexBuffer{createIndexBuffer(die.get())};
        std::shared_ptr<vulkan::Buffer> uniformBuffer{vulkan::Buffer::createUniformBuffer(m_device,
            sizeof (UniformBufferObject))};
        std::shared_ptr<vulkan::Buffer> uniformBufferPerObjFrag{vulkan::Buffer::createUniformBuffer(
            m_device, sizeof (PerObjectFragmentVariables))};
        std::shared_ptr<vulkan::DescriptorSet> descriptorSet = m_descriptorPools->allocateDescriptor();
        m_descriptorSetLayout->updateDescriptorSet(uniformBuffer, m_viewPointBuffer,
            uniformBufferPerObjFrag, descriptorSet, m_texture->getImageInfosForDescriptorSet());
        die->init(descriptorSet, indexBuffer, vertexBuffer, uniformBuffer, uniformBufferPerObjFrag);
    }
    updateDepthResources();

    initializeCommandBuffers();
}

void RainbowDiceVulkan::recreateSwapChain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(m_device->logicalDevice().get());

    cleanupSwapChain();

    m_swapChain.reset(new vulkan::SwapChain{m_device});

    m_depthImageView.reset(new vulkan::ImageView{vulkan::ImageFactory::createDepthImage(m_swapChain),
                                                 m_device->depthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT});
    updateDepthResources();

    m_renderPass.reset(new vulkan::RenderPass{m_device, m_swapChain});
    m_graphicsPipeline.reset(new vulkan::Pipeline{m_swapChain, m_renderPass, m_descriptorSetLayout,
                                            getBindingDescription(), getAttributeDescriptions(),
                                            SHADER_VERT_FILE, SHADER_FRAG_FILE});
    m_swapChainCommands.reset(new vulkan::SwapChainCommands{m_swapChain, m_commandPool, m_renderPass,
                                                            m_depthImageView});

    // trust what Java tells us the window size is instead of what Vulkan says it is because
    // Vulkan was found to be wrong in some cases.  0 for width or height, means trust the swap
    // chain values.
    if (width == 0 || height == 0) {
        updatePerspectiveMatrix(m_swapChain->extent().width, m_swapChain->extent().height);
    } else {
        updatePerspectiveMatrix(width, height);
    }

    for (auto const & die : dice) {
        die->updateUniformBuffer(m_proj, m_view);
    }

    initializeCommandBuffers();
}

void RainbowDiceVulkan::newSurface(std::shared_ptr<WindowType> surface) {
    auto atlas = m_texture->textureAtlas();
    m_texture.reset();
    m_swapChainCommands.reset();
    m_depthImageView.reset();
    m_imageAvailableSemaphore.reset();
    m_renderFinishedSemaphore.reset();
    m_viewPointBuffer.reset();
    m_commandPool.reset();
    m_graphicsPipeline.reset();
    for (auto const & die : dice) {
        die->destroyVulkanMemory();
    }
    m_descriptorPools.reset();
    m_descriptorSetLayout.reset();
    m_renderPass.reset();
    m_swapChain.reset();
    m_device.reset();
    m_instance.reset();


    m_instance = std::make_shared<vulkan::Instance>(std::move(surface));
    m_device = std::make_shared<vulkan::Device>(m_instance);
    m_swapChain = std::make_shared<vulkan::SwapChain>(m_device);
    m_renderPass = std::make_shared<vulkan::RenderPass>(m_device, m_swapChain);
    m_descriptorSetLayout = std::make_shared<DiceDescriptorSetLayout>(m_device);
    m_descriptorPools = std::make_shared<vulkan::DescriptorPools>(m_device, m_descriptorSetLayout);
    m_graphicsPipeline = std::make_shared<vulkan::Pipeline>(m_swapChain, m_renderPass, m_descriptorSetLayout,
                                            getBindingDescription(), getAttributeDescriptions(),
                                            SHADER_VERT_FILE, SHADER_FRAG_FILE);
    m_commandPool = std::make_shared<vulkan::CommandPool>(m_device);
    m_viewPointBuffer = vulkan::Buffer::createUniformBuffer(m_device, sizeof (glm::vec3));
    m_imageAvailableSemaphore = std::make_shared<vulkan::Semaphore>(m_device);
    m_renderFinishedSemaphore = std::make_shared<vulkan::Semaphore>(m_device);
    m_depthImageView = std::make_shared<vulkan::ImageView>(vulkan::ImageFactory::createDepthImage(m_swapChain),
                                           m_device->depthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
    m_swapChainCommands = std::make_shared<vulkan::SwapChainCommands>(m_swapChain, m_commandPool,
                                                      m_renderPass, m_depthImageView);

    m_viewPointBuffer->copyRawTo(&m_viewPoint, sizeof(m_viewPoint));

    setTexture(atlas);

    for (auto const &die : dice) {
        die->m_device = m_device;
    }
    initModels();
}

// call to finish loading the new models and their associated resources.
// it is assumed that the caller will first destroy the old models with destroyModels, then
// create the new ones, then call this function to recreate all the associated Vulkan resources.
void RainbowDiceVulkan::recreateModels() {
    m_commandPool.reset(new vulkan::CommandPool{m_device});
    m_swapChainCommands.reset(new vulkan::SwapChainCommands{m_swapChain, m_commandPool, m_renderPass,
        m_depthImageView});
    for (auto && die : dice) {
        die->loadModel(m_texture->textureAtlas());
        std::shared_ptr<vulkan::Buffer> vertexBuffer{createVertexBuffer(die.get())};
        std::shared_ptr<vulkan::Buffer> indexBuffer{createIndexBuffer(die.get())};
        std::shared_ptr<vulkan::Buffer> uniformBuffer{vulkan::Buffer::createUniformBuffer(m_device,
                sizeof (UniformBufferObject))};
        std::shared_ptr<vulkan::Buffer> uniformBufferPerObjFrag{vulkan::Buffer::createUniformBuffer(
                m_device, sizeof (PerObjectFragmentVariables))};
        std::shared_ptr<vulkan::DescriptorSet> descriptorSet = m_descriptorPools->allocateDescriptor();
        m_descriptorSetLayout->updateDescriptorSet(uniformBuffer, m_viewPointBuffer,
                uniformBufferPerObjFrag, descriptorSet, m_texture->getImageInfosForDescriptorSet());
        die->init(descriptorSet, indexBuffer, vertexBuffer, uniformBuffer, uniformBufferPerObjFrag);
    }

    initializeCommandBuffers();
}

std::shared_ptr<vulkan::Buffer> RainbowDiceVulkan::createVertexBuffer(Dice *die) {
    VkDeviceSize bufferSize = sizeof(die->die->getVertices()[0]) * die->die->getVertices().size();

    /* use a staging buffer in the CPU accessable memory to copy the data into graphics card
     * memory.  Then use a copy command to copy the data into fast graphics card only memory.
     */
    vulkan::Buffer stagingBuffer{m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.copyRawTo(die->die->getVertices().data(), static_cast<size_t>(bufferSize));

    std::shared_ptr<vulkan::Buffer> vertexBuffer{new vulkan::Buffer{m_device, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}};

    vertexBuffer->copyTo(m_commandPool, stagingBuffer, bufferSize);

    return vertexBuffer;
}

/* buffer for the indices - used to reference which vertex in the vertex array (by index) to
 * draw.  This way normally duplicated vertices would not need to be specified twice.
 * Only one index buffer per pipeline is allowed.  Put all dice in the same index buffer.
 */
std::shared_ptr<vulkan::Buffer>  RainbowDiceVulkan::createIndexBuffer(Dice *die) {
    VkDeviceSize bufferSize = sizeof(die->die->getIndices()[0]) * die->die->getIndices().size();

    vulkan::Buffer stagingBuffer{m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.copyRawTo(die->die->getIndices().data(), sizeof(die->die->getIndices()[0]) *
                                                           die->die->getIndices().size());

    std::shared_ptr<vulkan::Buffer> indexBuffer{new vulkan::Buffer{m_device, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}};

    indexBuffer->copyTo(m_commandPool, stagingBuffer, bufferSize);

    return indexBuffer;
}

void RainbowDiceVulkan::cleanupSwapChain() {
    m_swapChainCommands.reset();
    m_graphicsPipeline.reset();
    m_renderPass.reset();

    m_depthImageView.reset();

    m_swapChain.reset();
}

/* Allocate and record commands for each swap chain immage */
void RainbowDiceVulkan::initializeCommandBuffers() {
    /* begin recording commands into each comand buffer */
    for (size_t i = 0; i < m_swapChainCommands->size(); i++) {
        VkCommandBuffer commandBuffer = m_swapChainCommands->commandBuffer(i);
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        /* pInheritanceInfo is only used if flags includes:
         * VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
         * because that would mean that there is a secondary command buffer that will be used
         * in a single render pass
         */
        beginInfo.pInheritanceInfo = nullptr; // Optional

        /* this call will reset the command buffer.  its not possible to append commands at
         * a later time.
         */
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        /* begin the render pass: drawing starts here*/
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass->renderPass().get();
        renderPassInfo.framebuffer = m_swapChainCommands->frameBuffer(i);
        /* size of the render area */
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChain->extent();

        /* the color value to use when clearing the image with VK_ATTACHMENT_LOAD_OP_CLEAR,
         * using black with 0% opacity
         */
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
        clearValues[1].depthStencil = {1.0, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        /* begin recording commands - start by beginning the render pass.
         * none of these functions returns an error (they return void).  There will be no error
         * handling until recording is done.
         */
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        /* bind the graphics pipeline to the command buffer, the second parameter tells Vulkan
         * that we are binding to a graphics pipeline.
         */
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->pipeline().get());

        VkDeviceSize offsets[1] = {0};
        for (auto die : dice) {
            VkBuffer vertexBuffer = die->vertexBuffer->buffer().get();
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, die->indexBuffer->buffer().get(), 0, VK_INDEX_TYPE_UINT32);

            /* The MVP matrix and texture samplers */
            VkDescriptorSet descriptorSet = die->descriptorSet->descriptorSet().get();
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_graphicsPipeline->layout().get(), 0, 1, &descriptorSet, 0, nullptr);

            /* draw command:
             * parameter 1 - Command buffer for the draw command
             * parameter 2 - the vertex count
             * parameter 3 - the instance count, use 1 because we are not using instanced rendering
             * parameter 4 - offset into the vertex buffer
             * parameter 5 - offset for instance rendering
             */
            //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

            /* indexed draw command:
             * parameter 1 - Command buffer for the draw command
             * parameter 2 - the number of indices (the vertex count)
             * parameter 3 - the instance count, use 1 because we are not using instanced rendering
             * parameter 4 - offset into the index buffer
             * parameter 5 - offset to add to the indices in the index buffer
             * parameter 6 - offset for instance rendering
             */
            vkCmdDrawIndexed(commandBuffer, die->die->getIndices().size(), 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void RainbowDiceVulkan::drawFrame() {
    /* update the app state here */

    /* wait for presentation to finish before drawing the next frame.  Avoids a memory leak */
    vkQueueWaitIdle(m_device->presentQueue());

    uint32_t imageIndex;
    /* the third parameter is a timeout indicating how much time in nanoseconds we want to
     * wait for the image to become available (std::numeric_limits<uint64_t>::max() disables it.
     * an error from this function does not necessarily mean that we need to terminate
     * the program
     */
    VkResult result = vkAcquireNextImageKHR(m_device->logicalDevice().get(), m_swapChain->swapChain().get(),
        std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphore->semaphore().get(),
        VK_NULL_HANDLE, &imageIndex);

    /* If the window surface is no longer compatible with the swap chain, then we need to
     * recreate the swap chain and let the next call draw the image.
     * VK_SUBOPTIMAL_KHR means that the swap chain can still be used to present to the surface
     * but it no longer matches the window surface exactly.
     */
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(0, 0);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    /* wait for the semaphore before writing to the color attachment.  This means that we
     * could start executing the vertex shader before the image is available.
     */
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphore->semaphore().get()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    /* use the command buffer that corresponds to the image we just acquired */
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = m_swapChainCommands->commandBuffer(imageIndex);
    submitInfo.pCommandBuffers = &commandBuffer;

    /* indicate which semaphore to signal when execution is done */
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore->semaphore().get()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    /* the last parameter is a fence to indicate when execution is done, but we are using
     * semaphores instead so pass VK_NULL_HANDLE
     */
    if (vkQueueSubmit(m_device->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    /* submit the image back to the swap chain to have it eventually show up on the screen */
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    /* wait for the renderFinishedSemaphore to be signaled before presentation */
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    /* which swap chains to present the image to and the index of the image for
     * each swap chain
     */
    VkSwapchainKHR swapChains[] = {m_swapChain->swapChain().get()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    /* get an array of return codes for each swap chain.  Since there is only one, the
     * function return code can just be checked.
     */
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(m_device->presentQueue(), &presentInfo);

    /* If the window surface is no longer compatible with the swap chain
     * (VK_ERROR_OUT_OF_DATE_KHR), then we need to recreate the swap chain and let the next
     * call draw the image. VK_SUBOPTIMAL_KHR means that the swap chain can still be used
     * to present to the surface but it no longer matches the window surface exactly.
     * We recreate the swap chain in this case too.
     */
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain(0, 0);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    vkQueueWaitIdle(m_device->presentQueue());
}

void RainbowDiceVulkan::updateDepthResources() {
    m_depthImageView->image()->transitionImageLayout(m_device->depthFormat(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_commandPool);
}

std::shared_ptr<vulkan::Image> RainbowDiceVulkan::createTextureImage(uint32_t texWidth, uint32_t texHeight,
        std::unique_ptr<unsigned char[]> const &bitmap, size_t bitmapSize) {
    unsigned char const *buffer = bitmap.get();
    VkDeviceSize imageSize = bitmapSize;

    /* copy the image to CPU accessable memory in the graphics card.  Make sure that it has the
     * VK_BUFFER_USAGE_TRANSFER_SRC_BIT set so that we can copy from it to an image later
     */
    vulkan::Buffer stagingBuffer{m_device, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.copyRawTo(buffer, static_cast<size_t>(imageSize));

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    std::shared_ptr<vulkan::Image> textureImage{new vulkan::Image{m_device, texWidth, texHeight, format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}};

    /* transition the image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL. The image was created with
     * layout: VK_IMAGE_LAYOUT_UNDEFINED, so we use that to specify the old layout.
     */
    textureImage->transitionImageLayout(format, VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_commandPool);

    textureImage->copyBufferToImage(stagingBuffer, m_commandPool);

    /* transition the image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL so that the
     * shader can read from it.
     */
    textureImage->transitionImageLayout(format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_commandPool);

    return textureImage;
}

void RainbowDiceVulkan::loadObject(std::vector<std::string> const &symbols,
                                   std::vector<uint32_t> const &rerollIndices,
                                   std::vector<float> const &color) {
    glm::vec3 position(0.0f, 0.0f, -1.0f);
    std::shared_ptr<Dice> o(new Dice(m_device, symbols, rerollIndices, position, color));
    dice.push_back(o);
}

bool RainbowDiceVulkan::updateUniformBuffer() {
    for (DiceList::iterator iti = dice.begin(); iti != dice.end(); iti++) {
        if (!iti->get()->die->isStopped()) {
            for (DiceList::iterator itj = iti; itj != dice.end(); itj++) {
                if (!itj->get()->die->isStopped() && iti != itj) {
                    iti->get()->die->calculateBounce(itj->get()->die.get());
                }
            }
        }
    }

    bool needsRedraw = false;
    uint32_t i=0;

    auto const ext = m_swapChain->extent();
    float width = screenWidth(ext.width, ext.height);
    float height = screenHeight(ext.width, ext.height);
    for (auto && die : dice) {
        if (die->die->updateModelMatrix()) {
            needsRedraw = true;
        }
        if (die->die->isStopped() && !die->die->isStoppedAnimationStarted()) {
            uint32_t nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -width/2 + (2*stoppedX++ + 1) * DicePhysicsModel::stoppedRadius;
            float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die->animateMove(x, y);
        }
        die->updateUniformBuffer(m_proj, m_view);
        i++;
    }

    return needsRedraw;
}

void RainbowDiceVulkan::updateAcceleration(float x, float y, float z) {
    for (auto &&die : dice) {
        die->die->updateAcceleration(x, y, z);
    }
}

void RainbowDiceVulkan::resetPositions() {
    auto ext = m_swapChain->extent();
    float width = screenWidth(ext.width, ext.height);
    float height = screenHeight(ext.width, ext.height);
    for (auto &&die: dice) {
        die->die->resetPosition(width, height);
    }
}

void RainbowDiceVulkan::resetPositions(std::set<int> &diceIndices) {
    int i = 0;
    auto ext = m_swapChain->extent();
    float width = screenWidth(ext.width, ext.height);
    float height = screenHeight(ext.width, ext.height);
    for (auto &&die : dice) {
        if (diceIndices.find(i) != diceIndices.end()) {
            die->die->resetPosition(width, height);
        }
        i++;
    }
}

void RainbowDiceVulkan::addRollingDice() {
    auto ext = m_swapChain->extent();
    float width = screenWidth(ext.width, ext.height);
    float height = screenHeight(ext.width, ext.height);
    for (DiceList::iterator diceIt = dice.begin(); diceIt != dice.end(); diceIt++) {
        // Skip dice already being rerolled or does not get rerolled.
        if (diceIt->get()->isBeingReRolled || diceIt->get()->rerollIndices.size() == 0) {
            continue;
        }

        uint32_t result = diceIt->get()->die->getResult() % diceIt->get()->die->getNumberOfSymbols();
        bool shouldReroll = false;
        for (auto const &rerollIndex : diceIt->get()->rerollIndices) {
            if (result == rerollIndex) {
                shouldReroll = true;
                break;
            }
        }

        if (!shouldReroll) {
            continue;
        }

        glm::vec3 position(0.0f, 0.0f, -1.0f);
        std::shared_ptr<Dice> die(new Dice(m_device, diceIt->get()->die->getSymbols(),
                                           diceIt->get()->rerollIndices, position,
                                           diceIt->get()->die->dieColor()));
        diceIt->get()->isBeingReRolled = true;

        die->loadModel(m_texture->textureAtlas());
        std::shared_ptr<vulkan::Buffer> indexBuffer{createIndexBuffer(die.get())};
        std::shared_ptr<vulkan::Buffer> vertexBuffer{createVertexBuffer(die.get())};
        std::shared_ptr<vulkan::Buffer> uniformBuffer{vulkan::Buffer::createUniformBuffer(
                m_device, sizeof (UniformBufferObject))};
        std::shared_ptr<vulkan::Buffer> uniformBufferPerObjFrag{vulkan::Buffer::createUniformBuffer(
                m_device, sizeof (PerObjectFragmentVariables))};
        std::shared_ptr<vulkan::DescriptorSet> descriptorSet = m_descriptorPools->allocateDescriptor();
        m_descriptorSetLayout->updateDescriptorSet(uniformBuffer, m_viewPointBuffer,
                uniformBufferPerObjFrag, descriptorSet, m_texture->getImageInfosForDescriptorSet());
        die->init(descriptorSet, indexBuffer, vertexBuffer, uniformBuffer, uniformBufferPerObjFrag);
        die->die->resetPosition(width, height);
        die->updateUniformBuffer(m_proj, m_view);

        diceIt++;
        dice.insert(diceIt, die);
        diceIt--;
    }

    initializeCommandBuffers();

    int i = 0;
    for (auto &&die : dice) {
        if (die->die->isStopped()) {
            uint32_t nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -width/2 + (2*stoppedX++ + 1) * DicePhysicsModel::stoppedRadius;
            float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die->animateMove(x, y);
        }
        i++;
    }
}

bool RainbowDiceVulkan::allStopped() {
    for (auto &&die : dice) {
        if (!die->die->isStopped() || !die->die->isStoppedAnimationDone()) {
            return false;
        }
    }
    return true;
}

std::vector<std::vector<uint32_t >> RainbowDiceVulkan::getDiceResults() {
    std::vector<std::vector<uint32_t>> results;
    for (DiceList::iterator dieIt = dice.begin(); dieIt != dice.end(); dieIt++) {
        if (!dieIt->get()->die->isStopped()) {
            // Should not happen
            throw std::runtime_error("Not all die are stopped!");
        }
        std::vector<uint32_t > dieResults;
        while (dieIt->get()->isBeingReRolled) {
            dieResults.push_back(dieIt->get()->die->getResult());
            dieIt++;
        }
        dieResults.push_back(dieIt->get()->die->getResult());
        results.push_back(dieResults);
    }

    return results;
}

bool RainbowDiceVulkan::needsReroll() {
    for (auto const &die : dice) {
        if (die->needsReroll()) {
            return true;
        }
    }

    return false;
}

void RainbowDiceVulkan::resetToStoppedPositions(std::vector<uint32_t> const &upFaceIndices) {
    uint32_t i = 0;
    auto const ext = m_swapChain->extent();
    float width = screenWidth(ext.width, ext.height);
    float height = screenHeight(ext.width, ext.height);
    for (auto &&die : dice) {
        uint32_t nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
        uint32_t stoppedX = i%nbrX;
        uint32_t stoppedY = i/nbrX;
        float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
        float y = height/2 - (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;

        die->die->positionDice(upFaceIndices[i++], x, y);
        die->updateUniformBuffer(m_proj, m_view);
    }
}

/*
bool RainbowDiceVulkan::tapDice(float x, float y) {
    bool needsRedraw = false;
    auto const ext = m_swapChain->extent();
    float swidth = screenWidth(m_width, m_height);
    float sheight = screenHeight(m_width, m_height);
    float xnorm = x/(float)m_width * swidth - swidth / 2.0f;
    float ynorm = y/(float)m_height * sheight - sheight / 2.0f;
    float z1 = -1.0f;
    float z2 = 1.0f;

    glm::vec4 v1{xnorm, ynorm, z1, 1.0f};
    glm::vec4 v2{xnorm, ynorm, z2, 1.0f};

    glm::vec4 v1t = glm::inverse(m_view) * glm::inverse(m_proj) * v1;
    glm::vec4 v2t = glm::inverse(m_view) * glm::inverse(m_proj) * v2;

    glm::vec3 v1t3{v1t.x, v1t.y, v1t.z};
    v1t3 /= v1t.w;
    glm::vec3 v2t3{v2t.x, v2t.y, v2t.z};
    v2t3 /= v2t.w;

    float theta = (1.0f - v1t3.z)/(v2t3.z-v1t3.z);
    glm::vec3 pos = v1t3*(1-theta) + v2t3*theta;

    glm::vec3 position(0.0f, 0.0f, -1.0f);
    auto const &firstDie = dice.begin();
    std::shared_ptr<Dice> die(new Dice(m_device, firstDie->get()->die->getSymbols(),
                                       firstDie->get()->rerollIndices, position,
                                       firstDie->get()->die->dieColor()));

    die->loadModel(m_texture->textureAtlas());
    std::shared_ptr<vulkan::Buffer> indexBuffer{createIndexBuffer(die.get())};
    std::shared_ptr<vulkan::Buffer> vertexBuffer{createVertexBuffer(die.get())};
    std::shared_ptr<vulkan::Buffer> uniformBuffer{vulkan::Buffer::createUniformBuffer(
            m_device, sizeof (UniformBufferObject))};
    std::shared_ptr<vulkan::Buffer> uniformBufferPerObjFrag{vulkan::Buffer::createUniformBuffer(
            m_device, sizeof (PerObjectFragmentVariables))};
    std::shared_ptr<vulkan::DescriptorSet> descriptorSet = m_descriptorPools->allocateDescriptor();
    m_descriptorSetLayout->updateDescriptorSet(uniformBuffer, m_viewPointBuffer,
                                               uniformBufferPerObjFrag, descriptorSet, m_texture->getImageInfosForDescriptorSet());
    die->init(descriptorSet, indexBuffer, vertexBuffer, uniformBuffer, uniformBufferPerObjFrag);
    die->die->positionDice(0, pos.x, pos.y);
    die->updateUniformBufferFragmentVariables();
    die->updateUniformBuffer(m_proj, m_view);

    dice.push_back(die);
    initializeCommandBuffers();
    return true;
}
*/

// returns true if it needs a redraw
bool RainbowDiceVulkan::tapDice(float x, float y) {
    bool needsRedraw = false;
    auto const ext = m_swapChain->extent();
    float swidth = screenWidth(ext.width, ext.height);
    float sheight = screenHeight(ext.width, ext.height);
    float xnorm = x/m_width * swidth - swidth / 2.0f;
    float ynorm = y/m_height * sheight - sheight / 2.0f;
    glm::vec4 point1{xnorm, ynorm, 1.0f, 1.0f};
    glm::vec4 point2{xnorm, ynorm, -1.0f, 1.0f};

    glm::vec4 transformedP1 = glm::inverse(m_view) * glm::inverse(m_projWithoutPreTransform) * point1;
    glm::vec4 transformedP2 = glm::inverse(m_view) * glm::inverse(m_projWithoutPreTransform) * point2;

    glm::vec3 start = glm::vec3{transformedP1.x, transformedP1.y, transformedP1.z}/transformedP1.w;
    glm::vec3 line =  glm::vec3{transformedP2.x, transformedP2.y, transformedP2.z}/transformedP2.w - start;

    line = glm::normalize(line);
    for (auto const &die : dice) {
        float distanceToLine = glm::length(glm::cross(line, die->die->position() - start));
        if (distanceToLine < DicePhysicsModel::stoppedRadius) {
            die->toggleSelected();
            needsRedraw = true;
            break;
        }
    }

    return needsRedraw;
}