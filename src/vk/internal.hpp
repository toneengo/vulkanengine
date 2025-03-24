//here, the vulkan data (RenderContext), is a singleton object that is accessible via #include "interal.hpp"
#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include "types.hpp"
#include "destroy.hpp"
#include "descriptor.hpp"
#include <unordered_map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

constexpr uint32_t FRAME_OVERLAP = 2;

namespace engine {
    struct FrameContext {
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
        VkSemaphore swapchainSemaphore, renderSemaphore;
        VkFence renderFence;
        engine::DestroyQueue destroyQueue;
        engine::DescriptorAllocator descriptorAllocator;
    };

    inline struct RenderContext
    {
        bool initialised = false;
        VkDevice device;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamily;
        VmaAllocator allocator;
        std::unordered_map<VkImage, VmaAllocation> imageAllocations;
        std::unordered_map<VkBuffer, VmaAllocation> bufferAllocations;

        FrameContext frames[FRAME_OVERLAP];

        uint32_t frameIdx = 0;

        engine::DescriptorAllocator descriptorAllocator;

        //command buffer for immediate commands
        VkCommandPool immCommandPool;
        VkCommandBuffer immCommandBuffer;
        VkFence immCommandFence;

        GLFWwindow* window;
        GLFWmonitor* monitor;
        VkExtent2D windowExtent;
        VkExtent3D screenExtent; //desktop resolution
        float renderScale = 1.f;

        VkExtent2D extent; //framebufufer size
     
        struct Swapchain
        {
            VkSwapchainKHR swapchain;
            std::vector<VkImage> images;
            std::vector<VkImageView> imageViews;
            VkFormat imageFormat;
            VkExtent2D extent;
        };

        Framebuffer framebuffer; //main framebuffer
        Swapchain swapchain;
        engine::DestroyQueue destroyQueue;
    } ctx;

    inline FrameContext& get_frame() {
        return ctx.frames[ctx.frameIdx % FRAME_OVERLAP];
    }
    inline DestroyQueue destroyQueue;
#ifdef DBG
#define QUEUE_OBJ_DESTROY(x) do {\
        engine::Object o(x);\
        o.lineNumber = __LINE__;\
        o.fileName = __FILE__;\
        engine::destroyQueue.push(o);\
    } while(0)
#else
#define QUEUE_OBJ_DESTROY(x) destroyQueue.push(x);
#endif

}
