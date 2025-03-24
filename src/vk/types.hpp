#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
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
}
