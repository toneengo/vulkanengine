#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>
namespace engine
{
    struct Binding
    {
        uint32_t binding;
        VkDescriptorType type;
        uint32_t count;
    };

    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkExtent3D imageExtent;
        VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    };

    struct Buffer {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    //since we use dynamic rendering, VkFramebuffer isn't used, instead we have a collection of color/depth/stencil attchments
    struct Framebuffer
    {
        std::vector<Image> color;
        Image depth;
        Image stencil;
        uint32_t viewMask;
    };

    // holds the resources needed for a mesh
    struct GPUMeshBuffers {
        Buffer indexBuffer;
        Buffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    };

    struct Vertex {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    struct GeoSurface {
        uint32_t startIndex;
        uint32_t count;
    };

    struct MeshAsset {
        std::vector<GeoSurface> surfaces;
        GPUMeshBuffers meshBuffers;
    };

}
