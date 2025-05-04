//here, the vulkan data (RenderContext), is a singleton object that is accessible via #include "interal.hpp"
#pragma once
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "types.hpp"
#include "destroy.hpp"
#include "descriptor.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace spock {
    constexpr uint32_t FRAME_OVERLAP = 2;
    struct FrameContext {
        VkCommandPool               commandPool;
        VkCommandBuffer             commandBuffer;
        VkSemaphore                 swapchainSemaphore, renderSemaphore;
        VkFence                     renderFence;
        spock::DestroyQueue        destroyQueue;
        spock::DescriptorAllocator descriptorAllocator;
    };

    inline struct RenderContext {
        bool                        initialised = false;
        VkDevice                    device;
        VkInstance                  instance;
        VkDebugUtilsMessengerEXT    debugMessenger;
        VkSurfaceKHR                surface;
        VkPhysicalDevice            physicalDevice;
        VkQueue                     graphicsQueue;
        uint32_t                    graphicsQueueFamily;
        VmaAllocator                allocator;

        FrameContext                frames[FRAME_OVERLAP];

        uint32_t                    frameIdx = 0;

        spock::DescriptorAllocator descriptorAllocator;

        //command buffer for immediate commands
        VkCommandPool   immCommandPool;
        VkCommandBuffer immCommandBuffer;
        VkFence         immCommandFence;

        GLFWwindow*     window;
        GLFWmonitor*    monitor;
        VkExtent2D      windowExtent;
        VkExtent3D      screenExtent; //desktop resolution
        float           renderScale = 1.f;

        VkExtent2D      extent; //framebufufer size

        struct Swapchain {
            VkSwapchainKHR           swapchain;
            std::vector<VkImage>     images;
            std::vector<VkImageView> imageViews;
            VkFormat                 imageFormat;
            VkExtent2D               extent;
        };

        //settable by the "user"
        /*
        VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
        VkSampler currentSampler = VK_NULL_HANDLE;
        uint32_t textureDescriptorSetBinding = 0;
        uint32_t textureCount = 0;
        */

        Framebuffer          framebuffer; //main framebuffer
        Swapchain            swapchain;
        spock::DestroyQueue destroyQueue;
    } ctx;


    inline FrameContext&         get_frame() {
        return ctx.frames[ctx.frameIdx % FRAME_OVERLAP];
    }
    inline void         finish_frame() {
        ctx.frameIdx++;
    }
    inline DestroyQueue destroyQueue;

#ifdef DBG
#define QUEUE_DESTROY_OBJ(x)                                                                                                                                                       \
    do {                                                                                                                                                                           \
        spock::Object o(x);                                                                                                                                                       \
        o.lineNumber = __LINE__;                                                                                                                                                   \
        o.fileName   = __FILE__;                                                                                                                                                   \
        spock::destroyQueue.push(o);                                                                                                                                              \
    } while (0);
#else
#define QUEUE_DESTROY_OBJ(x) destroyQueue.push(x);
#endif

}
