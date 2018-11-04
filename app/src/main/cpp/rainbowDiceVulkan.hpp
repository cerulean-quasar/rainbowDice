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

    class Device {
    public:
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

        Device(std::shared_ptr<Instance> const &inInstance)
                : m_instance (inInstance),
                  m_physicalDevice{},
                  m_logicalDevice{},
                  m_graphicsQueue{},
                  m_presentQueue{},
                  m_depthFormat{} {
            pickPhysicalDevice();
            createLogicalDevice();
            m_depthFormat = findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                 VK_FORMAT_D24_UNORM_S8_UINT},
                                                VK_IMAGE_TILING_OPTIMAL,
                                                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        inline QueueFamilyIndices findQueueFamilies() {
            return findQueueFamilies(m_physicalDevice);
        }

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        inline SwapChainSupportDetails querySwapChainSupport() {
            return querySwapChainSupport(m_physicalDevice);
        }

        VkFormat depthFormat() { return m_depthFormat; }

        inline std::shared_ptr<VkDevice_T> const &logicalDevice() { return m_logicalDevice; }

        inline VkPhysicalDevice physicalDevice() { return m_physicalDevice; }

        inline VkQueue graphicsQueue() { return m_graphicsQueue; }

        inline VkQueue presentQueue() { return m_presentQueue; }

        inline std::shared_ptr<Instance> const &instance() { return m_instance; }

    private:
        /* ensure that the instance is not destroyed before the device by holding a shared
         * pointer to the instance here.
         */
        std::shared_ptr<Instance> m_instance;

        // the physical device does not need to be freed.
        VkPhysicalDevice m_physicalDevice;

        std::shared_ptr<VkDevice_T> m_logicalDevice;

        // the graphics and present queues are really part of the logical device and don't need to be freed.
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;

        VkFormat m_depthFormat;

        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                     VkImageTiling tiling, VkFormatFeatureFlags features);

        void pickPhysicalDevice();

        void createLogicalDevice();

        bool isDeviceSuitable(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    };

    class SwapChain {
    public:
        SwapChain(std::shared_ptr<Device> inDevice)
                : m_device(inDevice),
                  m_swapChain{VK_NULL_HANDLE},
                  m_imageFormat{},
                  m_extent{} {
            createSwapChain();
        }

        inline std::shared_ptr<Device> const &device() { return m_device; }
        inline std::shared_ptr<VkSwapchainKHR_T> const &swapChain() {return m_swapChain; }
        inline VkFormat imageFormat() { return m_imageFormat; }
        inline VkExtent2D extent() { return m_extent; }
    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<VkSwapchainKHR_T> m_swapChain;
        VkFormat m_imageFormat;
        VkExtent2D m_extent;

        void createSwapChain();

        VkSurfaceFormatKHR
        chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR
        chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    };

    class RenderPass {
    public:
        RenderPass(std::shared_ptr<Device> const &inDevice, std::shared_ptr<SwapChain> const &swapChain)
                : m_device{inDevice},
                  m_renderPass{} {
            createRenderPass(swapChain);
        }

        inline std::shared_ptr<VkRenderPass_T> const &renderPass() { return m_renderPass; }

    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<VkRenderPass_T> m_renderPass;

        void createRenderPass(std::shared_ptr<SwapChain> const &swapChain);
    };

    class DescriptorPools;

    class DescriptorSet {
        friend DescriptorPools;
    private:
        std::shared_ptr<DescriptorPools> m_descriptorPools;
        std::shared_ptr<VkDescriptorSet_T> m_descriptorSet;

        DescriptorSet(std::shared_ptr<DescriptorPools> inDescriptorPools,
                      std::shared_ptr<VkDescriptorSet_T> const &inDsc)
                : m_descriptorPools{inDescriptorPools},
                  m_descriptorSet{inDsc} {
        }

    public:
        inline std::shared_ptr<VkDescriptorSet_T> const &descriptorSet() { return m_descriptorSet; }
    };

    class Buffer;
    class DescriptorSetLayout {
    public:
        virtual std::shared_ptr<VkDescriptorSetLayout_T> const &descriptorSetLayout() = 0;
        virtual void updateDescriptorSet(std::shared_ptr<Buffer> const &uniformBuffer, std::shared_ptr<Buffer> const &viewPointBuffer,
                                 std::shared_ptr<vulkan::DescriptorSet> const &descriptorSet) = 0;
        virtual ~DescriptorSetLayout() {}
    };

    class DescriptorPool {
        friend DescriptorPools;
    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<VkDescriptorPool_T> m_descriptorPool;
        uint32_t m_totalDescriptorsInPool;
        uint32_t m_totalDescriptorsAllocated;

        DescriptorPool(std::shared_ptr<Device> const &inDevice, uint32_t totalDescriptors)
                : m_device{inDevice},
                  m_descriptorPool{},
                  m_totalDescriptorsInPool{totalDescriptors},
                  m_totalDescriptorsAllocated{0} {
            // no more descriptors in all the descriptor pools.  create another descriptor pool...
            std::array<VkDescriptorPoolSize, 3> poolSizes = {};

            // one for each wall and +3 for the floor, ball, and hole.
            poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = m_totalDescriptorsInPool;
            poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[1].descriptorCount = m_totalDescriptorsInPool;
            poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[2].descriptorCount = m_totalDescriptorsInPool;
            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = m_totalDescriptorsInPool;

            VkDescriptorPool descriptorPoolRaw;
            if (vkCreateDescriptorPool(m_device->logicalDevice().get(), &poolInfo, nullptr,
                                       &descriptorPoolRaw) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor pool!");
            }

            auto deleter = [inDevice](VkDescriptorPool descriptorPoolRaw) {
                vkDestroyDescriptorPool(inDevice->logicalDevice().get(), descriptorPoolRaw, nullptr);
            };

            m_descriptorPool.reset(descriptorPoolRaw, deleter);
        }

        VkDescriptorSet allocateDescriptor(std::shared_ptr<DescriptorSetLayout> const &layout) {
            if (m_totalDescriptorsAllocated == m_totalDescriptorsInPool) {
                return VK_NULL_HANDLE;
            } else {
                VkDescriptorSet descriptorSet;
                VkDescriptorSetLayout layouts[] = {layout->descriptorSetLayout().get()};
                VkDescriptorSetAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = m_descriptorPool.get();
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = layouts;

                /* the descriptor sets don't need to be freed because they are freed when the
                 * descriptor pool is freed
                 */
                int rc = vkAllocateDescriptorSets(m_device->logicalDevice().get(), &allocInfo,
                                                  &descriptorSet);
                if (rc != VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate descriptor set!");
                }
                m_totalDescriptorsAllocated++;
                return descriptorSet;
            }
        }

        bool hasAvailableDescriptorSets() {
            return m_totalDescriptorsAllocated < m_totalDescriptorsInPool;
        }
    };

/* for passing data other than the vertex data to the vertex shader */
    class DescriptorPools : public std::enable_shared_from_this<DescriptorPools> {
        friend DescriptorSet;
    private:
        std::shared_ptr<Device> m_device;
        static uint32_t constexpr m_numberOfDescriptorSetsInPool = 1024;
        std::shared_ptr<vulkan::DescriptorSetLayout> m_descriptorSetLayout;
        std::vector<DescriptorPool> m_descriptorPools;
        std::vector<VkDescriptorSet> m_unusedDescriptors;

    public:
        DescriptorPools(std::shared_ptr<Device> const &inDevice,
                        std::shared_ptr<DescriptorSetLayout> const &inDescriptorSetLayout)
                : m_device{inDevice},
                  m_descriptorSetLayout{inDescriptorSetLayout},
                  m_descriptorPools{},
                  m_unusedDescriptors{} {
        }

        std::shared_ptr<vulkan::DescriptorSetLayout> const &descriptorSetLayout() { return m_descriptorSetLayout; }

        std::shared_ptr<DescriptorSet> allocateDescriptor() {
            if (m_descriptorSetLayout == VK_NULL_HANDLE) {
                throw (std::runtime_error(
                        "DescriptorPool::allocateDescriptor - no descriptor set layout"));
            }

            auto deleter = [this](VkDescriptorSet descSet) {
                m_unusedDescriptors.push_back(descSet);
            };

            if (m_unusedDescriptors.size() > 0) {
                VkDescriptorSet descriptorSet = m_unusedDescriptors.back();
                m_unusedDescriptors.pop_back();

                return std::shared_ptr<DescriptorSet>{new DescriptorSet{shared_from_this(),
                                                                        std::shared_ptr<VkDescriptorSet_T>{descriptorSet, deleter}}};
            } else {
                for (auto &&descriptorPool : m_descriptorPools) {
                    if (descriptorPool.hasAvailableDescriptorSets()) {
                        VkDescriptorSet descriptorSet = descriptorPool.allocateDescriptor(
                                m_descriptorSetLayout);
                        return std::shared_ptr<DescriptorSet>{new DescriptorSet{shared_from_this(),
                                                                                std::shared_ptr<VkDescriptorSet_T>{descriptorSet, deleter}}};
                    }
                }


                DescriptorPool newDescriptorPool(m_device, m_numberOfDescriptorSetsInPool);
                m_descriptorPools.push_back(newDescriptorPool);
                VkDescriptorSet descriptorSet = newDescriptorPool.allocateDescriptor(
                        m_descriptorSetLayout);
                return std::shared_ptr<DescriptorSet>(new DescriptorSet(shared_from_this(),
                                                                        std::shared_ptr<VkDescriptorSet_T>(descriptorSet, deleter)));
            }
        }
    };

    class Shader {
        std::shared_ptr<Device> m_device;

        std::shared_ptr<VkShaderModule_T> m_shaderModule;

        void createShaderModule(std::string const &codeFile);

    public:
        Shader(std::shared_ptr<Device> const &inDevice, std::string const &codeFile)
                : m_device(inDevice),
                  m_shaderModule{} {
            createShaderModule(codeFile);
        }

        inline std::shared_ptr<VkShaderModule_T> const &shader() { return m_shaderModule; }
    };

    class Pipeline {
    public:
        Pipeline(std::shared_ptr<SwapChain> const &inSwapChain,
                 std::shared_ptr<RenderPass> const &inRenderPass,
                 std::shared_ptr<DescriptorSetLayout> const &inDescriptorSetLayout,
                 VkVertexInputBindingDescription const &bindingDescription,
                 std::vector<VkVertexInputAttributeDescription> const &attributeDescription,
                 std::string const &vertexShader,
                 std::string const &fragmentShader)
                : m_device{inSwapChain->device()},
                  m_swapChain{inSwapChain},
                  m_renderPass{inRenderPass},
                  m_descriptorSetLayout{inDescriptorSetLayout},
                  m_pipelineLayout{},
                  m_pipeline{} {
            createGraphicsPipeline(bindingDescription, attributeDescription, vertexShader,
                                   fragmentShader);
        }

        inline std::shared_ptr<VkPipeline_T> const &pipeline() { return m_pipeline; }
        inline std::shared_ptr<VkPipelineLayout_T> const &layout() { return m_pipelineLayout; }

        ~Pipeline() {
            m_pipeline.reset();
            m_pipelineLayout.reset();
        }

    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<SwapChain> m_swapChain;
        std::shared_ptr<RenderPass> m_renderPass;
        std::shared_ptr<DescriptorSetLayout> m_descriptorSetLayout;

        std::shared_ptr<VkPipelineLayout_T> m_pipelineLayout;
        std::shared_ptr<VkPipeline_T> m_pipeline;

        void createGraphicsPipeline(VkVertexInputBindingDescription const &bindingDescription,
                                    std::vector<VkVertexInputAttributeDescription> const &attributeDescriptions,
                                    std::string const &vertexShader, std::string const &fragmentShader);
    };

    class CommandPool {
    public:
        CommandPool(std::shared_ptr<Device> inDevice)
                : m_device{inDevice},
                  m_commandPool{} {
            createCommandPool();
        }

        inline std::shared_ptr<VkCommandPool_T> const &commandPool() { return m_commandPool; }
        inline std::shared_ptr<Device> const &device() { return m_device; }

    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<VkCommandPool_T> m_commandPool;

        void createCommandPool();
    };

    class SingleTimeCommands {
    public:
        SingleTimeCommands(std::shared_ptr<Device> inDevice,
                           std::shared_ptr<CommandPool> commandPool)
                : m_device{inDevice},
                  m_pool{commandPool},
                  m_commandBuffer{} {
            create();
        }

        void create();
        void begin();
        void end();

        inline std::shared_ptr<VkCommandBuffer_T> commandBuffer() { return m_commandBuffer; }
    private:
        std::shared_ptr<Device> m_device;
        std::shared_ptr<CommandPool> m_pool;
        std::shared_ptr<VkCommandBuffer_T> m_commandBuffer;
    };

    class Buffer {
    public:
        Buffer(std::shared_ptr<Device> inDevice, VkDeviceSize size, VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties)
                : m_device{inDevice},
                  m_buffer{},
                  m_bufferMemory{} {
            createBuffer(size, usage, properties);
        }

        void copyTo(std::shared_ptr<CommandPool> cmds, std::shared_ptr<Buffer> const &srcBuffer,
                    VkDeviceSize size) {
            copyTo(cmds, *srcBuffer, size);
        }

        void copyTo(std::shared_ptr<CommandPool> cmds, Buffer const &srcBuffer, VkDeviceSize size);

        void copyRawTo(void const *dataRaw, size_t size);

        inline std::shared_ptr<VkBuffer_T> const &buffer() const { return m_buffer; }
        inline std::shared_ptr<VkDeviceMemory_T> const &memory() const { return m_bufferMemory; }

        ~Buffer() {
            /* ensure order of destruction.
             * free the memory after the buffer has been destroyed because the buffer is bound to
             * the memory, so the buffer is still using the memory until the buffer is destroyed.
             */
            m_buffer.reset();
            m_bufferMemory.reset();
        }

        static std::shared_ptr<Buffer> createUniformBuffer(std::shared_ptr<Device> const &inDevice,
                                                           size_t size)
        {
            return std::shared_ptr<Buffer>{new vulkan::Buffer{inDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}};
        }
    private:
        std::shared_ptr<Device> m_device;

        std::shared_ptr<VkBuffer_T> m_buffer;
        std::shared_ptr<VkDeviceMemory_T> m_bufferMemory;

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    };

} /* namespace vulkan */

VkVertexInputBindingDescription getBindingDescription();
std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

class DiceDescriptorSetLayout : public vulkan::DescriptorSetLayout {
public:
    DiceDescriptorSetLayout(std::shared_ptr<vulkan::Device> const &inDevice)
            : m_device{inDevice},
              m_descriptorSetLayout{}
    {
        createDescriptorSetLayout();
    }

    void updateDescriptorSet(std::shared_ptr<vulkan::Buffer> const &uniformBuffer,
                             std::shared_ptr<vulkan::Buffer> const &viewPointBuffer,
                             std::shared_ptr<vulkan::DescriptorSet> const &descriptorSet);

    inline std::shared_ptr<VkDescriptorSetLayout_T> const &descriptorSetLayout() { return m_descriptorSetLayout; }
private:
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<VkDescriptorSetLayout_T> m_descriptorSetLayout;

    void createDescriptorSetLayout();
};

class RainbowDiceVulkan : public RainbowDice {
public:
    RainbowDiceVulkan(WindowType *window)
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
              swapChainImages(), swapChainImageViews(), swapChainFramebuffers()
    {
        // copy the view point of the scene into device memory to send to the fragment shader for the
        // Blinn-Phong lighting model.  Copy it over here too since it is a constant.
        m_viewPointBuffer->copyRawTo(&viewPoint, sizeof(viewPoint));
    }

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
    static std::string const SHADER_VERT_FILE;
    static std::string const SHADER_FRAG_FILE;

    std::shared_ptr<vulkan::Instance> m_instance;
    std::shared_ptr<vulkan::Device> m_device;
    std::shared_ptr<vulkan::SwapChain> m_swapChain;
    std::shared_ptr<vulkan::RenderPass> m_renderPass;

    /* for passing data other than the vertex data to the vertex shader */
    std::shared_ptr<vulkan::DescriptorSetLayout> m_descriptorSetLayout;
    std::shared_ptr<vulkan::DescriptorPools> m_descriptorPools;

    std::shared_ptr<vulkan::Pipeline> m_graphicsPipeline;
    std::shared_ptr<vulkan::CommandPool> m_commandPool;
    std::shared_ptr<vulkan::Buffer> m_viewPointBuffer;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;


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
        Dice(std::shared_ptr<vulkan::Device> const &inDevice,
             std::vector<std::string> &symbols, glm::vec3 position)
                : m_device{inDevice},
                  die(nullptr)
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

    };

    typedef std::list<std::shared_ptr<Dice> > DiceList;
    DiceList dice;

    std::vector<VkFramebuffer> swapChainFramebuffers;

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

    std::shared_ptr<vulkan::Buffer> createVertexBuffer(Dice *die);
    std::shared_ptr<vulkan::Buffer> createIndexBuffer(Dice *die);
    void updatePerspectiveMatrix();

    void cleanupSwapChain();
    void createImageViews();
    void createFramebuffers();
    void createCommandBuffers();
    void createSemaphores();
    void createDepthResources();
    bool hasStencilComponent(VkFormat format);
    void createTextureImages();
    void createTextureImage(VkImage &textureImage, VkDeviceMemory &textureImageMemory);
    void createTextureSampler(VkSampler &textureSampler);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};
#endif
