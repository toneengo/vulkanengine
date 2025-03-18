#pragma once
#include "vk_mem_alloc.h"
#include "lib/ds.hpp"
#include "descriptor.hpp"
#include "objects.hpp"
#include <vulkan/vulkan_core.h>

struct VulkanContext;

//general object struct, allows for putting any type of object into one array/vector
struct Object
{
    union
    {
        AllocatedImage image;
        VkDescriptorSetLayout dsl;
        VkDescriptorPool dp;
        VkPipelineLayout pll;
        VkPipeline pl;
        VkFence fence;
        VkCommandPool commandPool;
        AllocatedBuffer buffer;
        VkSampler sampler;
        DescriptorAllocatorGrowable* dag;
    };
    VKOBJ type;
    VulkanContext* ctx;
    Object() : type(VKOBJ::None) {}
    Object(AllocatedImage _im) : image(_im), type(VKOBJ::AllocatedImage) {}
    Object(VmaAllocator _allocator) : type(VKOBJ::Allocator) {}
    Object(VkDescriptorPool _dp) : dp(_dp), type(VKOBJ::DescriptorPool) {}
    Object(VkDescriptorSetLayout _dsl) : dsl(_dsl), type(VKOBJ::DescriptorSetLayout) {}
    Object(VkPipelineLayout _pll) : pll(_pll), type(VKOBJ::PipelineLayout) {}
    Object(VkPipeline _pl) : pl(_pl), type(VKOBJ::Pipeline) {}
    Object(VkFence _fence) : fence(_fence), type(VKOBJ::Fence) {}
    Object(VkCommandPool _commandPool) : commandPool(_commandPool), type(VKOBJ::CommandPool) {}
    Object(AllocatedBuffer _buffer) : buffer(_buffer), type(VKOBJ::AllocatedBuffer) {}
    Object(VkSampler _sampler) : sampler(_sampler), type(VKOBJ::Sampler) {}
    Object(DescriptorAllocatorGrowable* _dag) : dag(_dag), type(VKOBJ::DescriptorAllocatorGrowable) {}

    void destroy();
};

class DeletionQueue {
friend class VulkanContext;
public:
    void flush();
    void push(Object _o);
private:
    VulkanContext* ctx;
    DynamicArray<Object> queue;
};

