#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <functional>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "lib/ds.hpp"
#include "descriptor.hpp"
#include "objects.hpp"
#include "del_queue.hpp"
#include <glm/glm.hpp>
#include "loader.hpp"

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};
struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;

    DeletionQueue frameDeletionQueue;
    DescriptorAllocatorGrowable frameDescriptors;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
    const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
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
    void init_default_data();

    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    void destroy_image(const AllocatedImage& img);

    void resize_swapchain();

    void draw_geometry(VkCommandBuffer cmd);
    void render_loop();
    void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
    void cleanup();
	FrameData& get_current_frame() { return frames[frameNumber % FRAME_OVERLAP]; };
    VkInstance get_instance() { return instance; }

    void immediate_submit(std::function<void(VkCommandBuffer)> function);
    GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

private:
    GLFWwindow* window;
    std::vector<std::shared_ptr<MeshAsset>> testMeshes;

    struct {
        int width;
        int height;
    } windowInfo;

    bool initialised = false;
    VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;

    VkPipelineLayout _meshPipelineLayout;
    VkPipeline _meshPipeline;
    GPUMeshBuffers rectangle;
    void init_mesh_pipeline();

    uint32_t frameNumber = 0;
	FrameData frames[FRAME_OVERLAP];
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

    VkDevice device;
    VkPhysicalDevice chosenGPU;
    VkSurfaceKHR surface;

    VkPipelineLayout trianglePipelineLayout;
    VkPipeline trianglePipeline;
    void init_triangle_pipeline();

    //draw resources
    VmaAllocator allocator;
	AllocatedImage drawImage;
    AllocatedImage depthImage;
	VkExtent2D drawExtent;
    float renderScale = 1.f;

    AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

    VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;

	VkPipelineLayout gradientPipelineLayout;

    DeletionQueue mainDeletionQueue;
    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void destroy_buffer(const AllocatedBuffer& buffer);

	DescriptorAllocatorGrowable globalDescriptorAllocator;
	VkDescriptorSet drawImageDescriptors;
	VkDescriptorSetLayout drawImageDescriptorLayout;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkExtent2D swapchainExtent;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance;

    std::vector<ComputeEffect> backgroundEffects;
    int currentBackgroundEffect = 0;

    VkFence immFence;
    VkCommandBuffer immCommandBuffer;
    VkCommandPool immCommandPool;
    VkDescriptorSetLayout _singleImageDescriptorLayout;

    GPUSceneData sceneData;
    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
};
