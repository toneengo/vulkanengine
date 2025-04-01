#pragma once
#include <vulkan/vulkan_core.h>
#include "types.hpp"
#include "vk_mem_alloc.h"
namespace spock {
    //general object struct, allows for putting any type of object into one array
    struct Object {
        enum class OBJ {
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

        union {
            VkImage               image;
            VkImageView           imageView;
            VkDescriptorSetLayout dsl;
            VkDescriptorPool      dp;
            VkPipelineLayout      pll;
            VkPipeline            pl;
            VkFence               fence;
            VkCommandPool         commandPool;
            VkBuffer              buffer;
            VkSampler             sampler;
        };
        VmaAllocation allocation;
        OBJ           type;
#ifdef DBG
        int         lineNumber = -1;
        const char* fileName;
#endif
        Object() : type(OBJ::None) {}
        Object(spock::Image _im) : image(_im.image), allocation(_im.allocation), type(OBJ::Image) {}
        Object(VkImageView _imV) : imageView(_imV), type(OBJ::ImageView) {}
        Object(VmaAllocator _allocator) : type(OBJ::Allocator) {}
        Object(VkDescriptorPool _dp) : dp(_dp), type(OBJ::DescriptorPool) {}
        Object(VkDescriptorSetLayout _dsl) : dsl(_dsl), type(OBJ::DescriptorSetLayout) {}
        Object(VkPipelineLayout _pll) : pll(_pll), type(OBJ::PipelineLayout) {}
        Object(VkPipeline _pl) : pl(_pl), type(OBJ::Pipeline) {}
        Object(VkFence _fence) : fence(_fence), type(OBJ::Fence) {}
        Object(VkCommandPool _commandPool) : commandPool(_commandPool), type(OBJ::CommandPool) {}
        Object(spock::Buffer _buffer) : buffer(_buffer.buffer), allocation(_buffer.allocation), type(OBJ::Buffer) {}
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
