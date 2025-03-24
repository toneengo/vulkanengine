#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
namespace engine {
    //general object struct, allows for putting any type of object into one array
    struct Object
    {
        enum class OBJ
        {
            None,
            Image,
            ImageView,
            Allocator,
            DescriptorPool,
            DescriptorSetLayout,
            PipelineLayout,
            Pipeline,
            Fence,
            CommandPool,
            Buffer,
            Sampler,
        };

        union
        {
            VkImage image;
            VkImageView imageView;
            VkDescriptorSetLayout dsl;
            VkDescriptorPool dp;
            VkPipelineLayout pll;
            VkPipeline pl;
            VkFence fence;
            VkCommandPool commandPool;
            VkBuffer buffer;
            VkSampler sampler;
        };
        OBJ type;
#ifdef DBG
        int lineNumber;
        const char* fileName;
#endif
        Object() : type(OBJ::None) {}
        Object(VkImage _im) : image(_im), type(OBJ::Image) {}
        Object(VkImageView _imV) : imageView(_imV), type(OBJ::ImageView) {}
        Object(VmaAllocator _allocator) : type(OBJ::Allocator) {}
        Object(VkDescriptorPool _dp) : dp(_dp), type(OBJ::DescriptorPool) {}
        Object(VkDescriptorSetLayout _dsl) : dsl(_dsl), type(OBJ::DescriptorSetLayout) {}
        Object(VkPipelineLayout _pll) : pll(_pll), type(OBJ::PipelineLayout) {}
        Object(VkPipeline _pl) : pl(_pl), type(OBJ::Pipeline) {}
        Object(VkFence _fence) : fence(_fence), type(OBJ::Fence) {}
        Object(VkCommandPool _commandPool) : commandPool(_commandPool), type(OBJ::CommandPool) {}
        Object(VkBuffer _buffer) : buffer(_buffer), type(OBJ::Buffer) {}
        Object(VkSampler _sampler) : sampler(_sampler), type(OBJ::Sampler) {}

        void destroy();
    };

    class DestroyQueue {
    public:
        void flush();
        void push(Object _o);
    private:
        std::vector<Object> queue;
    };

}
