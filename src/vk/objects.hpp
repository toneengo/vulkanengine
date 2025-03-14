#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

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
    CommandPool
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

