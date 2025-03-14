#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "lib/ds.hpp"
#include "descriptor.hpp"
#include "objects.hpp"
#include "del_queue.hpp"
#include <glm/glm.hpp>

struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;

    DeletionQueue frameDeletionQueue;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

using ImmFn = void(*)(VkCommandBuffer cmd);

constexpr unsigned int FRAME_OVERLAP = 2;

struct GLFWwindow;
class VulkanContext {
    friend class DeletionQueue;
    friend class Object;
public:
    void init();
    void init_vulkan();
    void init_swapchain();
    void init_glfw();
    void init_commands();
    void init_sync_structures();
    void init_descriptors();
    void init_imgui();
    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
    void draw();
    void draw_background(VkCommandBuffer cmd);
    void init_pipelines();
	void init_background_pipelines();

    void render_loop();
    void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
    void cleanup();
	FrameData& get_current_frame() { return frames[frameNumber % FRAME_OVERLAP]; };
    VkInstance get_instance() { return instance; }

    void immediate_submit(ImmFn);

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

    VkPipeline gradientPipeline;
	VkPipelineLayout gradientPipelineLayout;

    DeletionQueue mainDeletionQueue;

	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet drawImageDescriptors;
	VkDescriptorSetLayout drawImageDescriptorLayout;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance;

    VkFence immFence;
    VkCommandBuffer immCommandBuffer;
    VkCommandPool immCommandPool;
};
