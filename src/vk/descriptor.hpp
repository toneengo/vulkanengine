#pragma once
#include <vulkan/vulkan.hpp>

namespace engine {
    struct DescriptorAllocator
    {
        struct PoolSizeRatio {
            VkDescriptorType type;
            float ratio;
        };

        bool initialised = false;
        std::vector<PoolSizeRatio> ratios;
        std::vector<VkDescriptorPool> pools;
        uint32_t currentPool = 0;
        uint32_t setsPerPool = 0;

        void init(std::initializer_list<PoolSizeRatio> _ratios, uint32_t maxSets);
        VkDescriptorPool create_pool();
        void clear_pools();
        void destroy_pools();
        VkDescriptorSet allocate(VkDescriptorSetLayout layout);
    };
}
