#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "lib/ds.hpp"

struct DescriptorLayoutBuilder {

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

enum class VKOBJ
{
    None,
    AllocatedImage,
    Allocator
};
struct object
{
    union
    {
        AllocatedImage image;
    };
    VKOBJ type;
    object() : type(VKOBJ::None) {}
    object(AllocatedImage _im) : image(_im), type(VKOBJ::AllocatedImage) {}
    object(VmaAllocator _allocator) : type(VKOBJ::Allocator) {}
    void destroy(VkDevice _device, VmaAllocator _allocator)
    {
        switch (type)
        {
            case VKOBJ::AllocatedImage:
                vkDestroyImageView(_device, image.imageView, nullptr);
                vmaDestroyImage(_allocator, image.image, image.allocation);
                break;
            case VKOBJ::Allocator:
                vmaDestroyAllocator(_allocator);
                break;
            default:
                break;
        }
    }
};
struct DeletionQueue {
    DynamicArray<object> queue;
    void flush(VkDevice _device, VmaAllocator _allocator) {
        for (int i = queue.size; i >= 0; i--)
        {
            queue[i].destroy(_device, _allocator);
        }
        queue.clear();
    }
};
struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;

    DeletionQueue frameDeletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

struct GLFWwindow;
class VulkanContext {
public:
    void init();
    void init_vulkan();
    void init_swapchain();
    void init_glfw();
    void init_commands();
    void init_sync_structures();
    void draw();
    void draw_background(VkCommandBuffer cmd);

    void render_loop();
    void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
    void cleanup();
	FrameData& get_current_frame() { return frames[frameNumber % FRAME_OVERLAP]; };
    VkInstance get_instance() { return instance; }

private:
    GLFWwindow* window;

    struct {
        uint32_t width;
        uint32_t height;
    } windowInfo;

    bool initialised = false;
    VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;

    uint32_t frameNumber = 0;
	FrameData frames[FRAME_OVERLAP];
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

    VkDevice device;
    VkPhysicalDevice chosenGPU;
    VkSurfaceKHR surface;

    //draw resources
    VmaAllocator allocator;
	AllocatedImage drawImage;
	VkExtent2D drawExtent;

    DeletionQueue mainDeletionQueue;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance;
};
