#pragma once
#include <vulkan/vulkan.hpp>
#include "vk/descriptor.hpp"
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>

enum class VKOBJ
{
    None,
    AllocatedImage,
    Allocator,
    DescriptorPool,
    DescriptorSetLayout,
    PipelineLayout,
    Pipeline,
    Fence,
    CommandPool,
    AllocatedBuffer,
    Sampler,
    DescriptorAllocatorGrowable,
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex {

	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers {

    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

