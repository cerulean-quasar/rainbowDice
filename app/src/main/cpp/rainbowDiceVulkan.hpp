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

namespace vulkan {
#define DEBUG
#ifdef DEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    const std::vector<const char *> validationLayers = {
            /* required for checking for errors and getting error messages */
            //"VK_LAYER_LUNARG_standard_validation"
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects"
    };

    class VulkanLibrary {
    public:
        VulkanLibrary() {
            if (!loadVulkan()) {
                throw std::runtime_error("Could not find vulkan library.");
            }
        }
    };

    class Instance {
    public:
        Instance(WindowType *inWindow)
                : m_loader{},
                  m_window{},
                  m_instance{},
                  m_callback{},
                  m_surface{} {
            auto deleter = [](WindowType *windowRaw) {
                /* release the java window object */
                if (windowRaw != nullptr) {
                    ANativeWindow_release(windowRaw);
                }
            };

            m_window.reset(inWindow, deleter);

            createInstance();
            setupDebugCallback();
            createSurface();
        }

        inline std::shared_ptr<VkInstance_T> const &instance() { return m_instance; }
        inline std::shared_ptr<VkSurfaceKHR_T> const &surface() { return m_surface; }
        inline std::shared_ptr<VkDebugReportCallbackEXT_T> const &callback() { return m_callback; }

    private:
        VulkanLibrary m_loader;
        std::shared_ptr<WindowType> m_window;
        std::shared_ptr<VkInstance_T> m_instance;
        std::shared_ptr<VkDebugReportCallbackEXT_T> m_callback;
        std::shared_ptr<VkSurfaceKHR_T> m_surface;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugReportFlagsEXT flags,
                VkDebugReportObjectTypeEXT objType,
                uint64_t obj,
                size_t location,
                int32_t code,
                const char *layerPrefix,
                const char *msg,
                void *userData) {

            std::cerr << "validation layer: " << msg << std::endl;

            return VK_FALSE;
        }

        void createSurface();

        void setupDebugCallback();

        void createInstance();

        bool checkExtensionSupport();

        bool checkValidationLayerSupport();

        std::vector<const char *> getRequiredExtensions();

        VkResult createDebugReportCallbackEXT(
                const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                const VkAllocationCallbacks *pAllocator,
                VkDebugReportCallbackEXT *pCallback);

        static void
        destroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback,
                                      const VkAllocationCallbacks *pAllocator);
    };

} /* namespace vulkan */


class RainbowDiceVulkan : public RainbowDice {
public:
    RainbowDiceVulkan(WindowType *window)
            : m_instance{new vulkan::Instance{window}},
              swapChainImages(), swapChainImageViews(), swapChainFramebuffers() {}
    virtual void initWindow(WindowType *window);

    virtual void initPipeline();

    virtual void initThread() {}

    virtual void cleanupThread() {}

    virtual void destroyWindow() {}

    virtual void cleanup();

    virtual void drawFrame();

    virtual bool updateUniformBuffer();

    virtual bool allStopped();

    virtual std::vector<std::string> getDiceResults();

    virtual void loadObject(std::vector<std::string> &symbols);

    virtual void destroyModels();

    virtual void recreateModels();

    virtual void recreateSwapChain();

    virtual void updateAcceleration(float x, float y, float z);

    virtual void resetPositions();

    virtual void resetPositions(std::set<int> &dice);

    virtual void resetToStoppedPositions(std::vector<std::string> const &symbols);

    virtual void addRollingDiceAtIndices(std::set<int> &diceIndices);

    virtual ~RainbowDiceVulkan() { }
private:
    std::shared_ptr<vulkan::Instance> m_instance;

    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        int presentFamily = -1;

        bool isComplete() {
            return graphicsFamily >= 0 && presentFamily >= 0;
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class DescriptorPools;
    class DescriptorPool {
        friend DescriptorPools;
    private:
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        uint32_t totalDescriptorsAllocated;
        uint32_t totalDescriptorsInPool;

        DescriptorPool(VkDescriptorPool pool, uint32_t totalDescriptors)
                : descriptorPool(pool), totalDescriptorsInPool(totalDescriptors),
                  totalDescriptorsAllocated(0)
        { }
        VkDescriptorSet allocateDescriptor(VkDescriptorSetLayout layout) {
            if (totalDescriptorsAllocated == totalDescriptorsInPool) {
                return VK_NULL_HANDLE;
            } else {
                VkDescriptorSet descriptorSet;
                VkDescriptorSetLayout layouts[] = {layout};
                VkDescriptorSetAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = descriptorPool;
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = layouts;

                /* the descriptor sets don't need to be freed because they are freed when the
                 * descriptor pool is freed
                 */
                int rc = vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet);
                if (rc != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate descriptor set!");
                }
                totalDescriptorsAllocated++;
                return descriptorSet;
            }
        }

        bool hasAvailableDescriptorSets() { return totalDescriptorsAllocated < totalDescriptorsInPool; }

    };

    /* for passing data other than the vertex data to the vertex shader */
    class DescriptorPools {
    private:
        uint32_t const numberOfDescriptorSetsInPool = 128;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<DescriptorPool> descriptorPools;
        std::vector<VkDescriptorSet> unusedDescriptors;
    public:
        DescriptorPools() : descriptorSetLayout(VK_NULL_HANDLE) { }
        ~DescriptorPools() { destroyResources(); }

        void setDescriptorSetLayout(VkDescriptorSetLayout layout) {
            descriptorSetLayout = layout;
        }
        VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; }
        void destroyResources() {
            unusedDescriptors.clear();

            for (auto &&descriptorPool: descriptorPools) {
                vkDestroyDescriptorPool(logicalDevice, descriptorPool.descriptorPool, nullptr);
            }

            descriptorPools.clear();

            if (descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
                descriptorSetLayout = VK_NULL_HANDLE;
            }

        }
        void freeDescriptor(VkDescriptorSet descriptorSet) {
            unusedDescriptors.push_back(descriptorSet);
        }

        VkDescriptorSet allocateDescriptor() {
            if (descriptorSetLayout == VK_NULL_HANDLE) {
                throw (std::runtime_error("DescriptorPool::allocateDescriptor - no descriptor set layout"));
            }

            if (unusedDescriptors.size() > 0) {
                VkDescriptorSet descriptorSet = unusedDescriptors.back();
                unusedDescriptors.pop_back();
                return descriptorSet;
            } else {
                for (auto &&descriptorPool : descriptorPools) {
                    if (descriptorPool.hasAvailableDescriptorSets()) {
                        return descriptorPool.allocateDescriptor(descriptorSetLayout);
                    }
                }

                // no more descriptors in all the descriptor pools.  create another descriptor pool...
                std::array<VkDescriptorPoolSize, 3> poolSizes = {};

                // one for each wall and +3 for the floor, ball, and hole.
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = numberOfDescriptorSetsInPool;
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = numberOfDescriptorSetsInPool;
                poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[2].descriptorCount = numberOfDescriptorSetsInPool;
                VkDescriptorPoolCreateInfo poolInfo = {};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                poolInfo.pPoolSizes = poolSizes.data();
                poolInfo.maxSets = numberOfDescriptorSetsInPool;

                VkDescriptorPool descriptorPool;
                if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create descriptor pool!");
                }

                DescriptorPool newDescriptorPool(descriptorPool, numberOfDescriptorSetsInPool);

                descriptorPools.push_back(newDescriptorPool);
                return newDescriptorPool.allocateDescriptor(descriptorSetLayout);
            }
        }
    };
    /* for passing data other than the vertex data to the vertex shader */
    DescriptorPools descriptorPools;

    static VkDevice logicalDevice;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkBuffer viewPointBuffer;
    VkDeviceMemory viewPointBufferMemory;

    struct Dice {
        std::shared_ptr<DicePhysicsModel> die;

        /* vertex buffer and index buffer. the index buffer indicates which vertices to draw and in
         * the specified order.  Note, vertices can be listed twice if they should be part of more
         * than one triangle.
         */
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        /* for passing data other than the vertex data to the vertex shader */
        VkDescriptorSet descriptorSet;
        VkBuffer uniformBuffer;
        VkDeviceMemory uniformBufferMemory;

        bool isBeingReRolled;
        Dice(std::vector<std::string> &symbols, glm::vec3 position) : die(nullptr)
        {
            isBeingReRolled = false;
            long nbrSides = symbols.size();
            if (nbrSides == 4) {
                die.reset(new DiceModelTetrahedron(symbols, position));
            } else if (nbrSides == 12) {
                die.reset(new DiceModelDodecahedron(symbols, position));
            } else if (nbrSides == 20) {
                die.reset(new DiceModelIcosahedron(symbols, position));
            } else if (6 % nbrSides == 0) {
                die.reset(new DiceModelCube(symbols, position));
            } else {
                die.reset(new DiceModelHedron(symbols, position));
            }
        }

        void loadModel(int width, int height) {
            die->loadModel();
            die->setView();
            die->updatePerspectiveMatrix(width, height);
        }

        void init(VkDescriptorSet inDescriptorSet, VkBuffer inIndexBuffer, VkDeviceMemory inIndexBufferMemory, VkBuffer inVertexBuffer, VkDeviceMemory inVertexBufferMemory, VkBuffer inUniformBuffer, VkDeviceMemory inUniformBufferMemory) {
            uniformBuffer = inUniformBuffer;
            uniformBufferMemory = inUniformBufferMemory;
            indexBuffer = inIndexBuffer;
            indexBufferMemory = inIndexBufferMemory;
            vertexBuffer = inVertexBuffer;
            vertexBufferMemory = inVertexBufferMemory;
            descriptorSet = inDescriptorSet;
        }

        ~Dice() {
            /* free the memory after the buffer has been destroyed because the buffer is bound to 
             * the memory, so the buffer is still using the memory until the buffer is destroyed.
             */
            vkDestroyBuffer(logicalDevice, uniformBuffer, nullptr);
            vkFreeMemory(logicalDevice, uniformBufferMemory, nullptr);

            vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
            vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);

            vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
            vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
        }

        void updateUniformBuffer() {
            void* data;
            vkMapMemory(logicalDevice, uniformBufferMemory, 0, sizeof(die->ubo), 0, &data);
            memcpy(data, &die->ubo, sizeof(die->ubo));
            vkUnmapMemory(logicalDevice, uniformBufferMemory);
        }

    };

    typedef std::list<std::shared_ptr<Dice> > DiceList;
    DiceList dice;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    /* use semaphores to coordinate the rendering and presentation. Could also use fences
     * but fences are more for coordinating in our program itself and not for internal
     * Vulkan coordination of resource usage.
     */
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    /* depth buffer image */
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    WindowType *window = nullptr;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    std::vector<char> readFile(const std::string &filename);

    void createUniformBuffer(VkBuffer &uniformBuffer, VkDeviceMemory &uniformBufferMemory);
    void createVertexBuffer(Dice *die, VkBuffer &vertexBuffer, VkDeviceMemory &vertexBufferMemory);
    void createIndexBuffer(Dice *die, VkBuffer &indexBuffer, VkDeviceMemory &indexBufferMemory);
    void updatePerspectiveMatrix();

    void cleanupSwapChain();
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createCommandPool();
    void createCommandBuffers();
    void createSemaphores();
    void createDepthResources();
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    bool hasStencilComponent(VkFormat format);
    void createDescriptorSet(VkBuffer uniformBuffer, VkBuffer viewPointBuffer, VkDescriptorSet &descriptorSet);
    //void createDescriptorPool();
    void createDescriptorSetLayout(VkDescriptorSetLayout &descriptorSetLayout);
    void createTextureImages();
    void createTextureImage(VkImage &textureImage, VkDeviceMemory &textureImageMemory);
    void createTextureSampler(VkSampler &textureSampler);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};
#endif
