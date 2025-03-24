#include "descriptor.hpp"
#include "internal.hpp"
#include "lib/util.hpp"

using namespace engine;

void DescriptorAllocator::init(std::initializer_list<DescriptorAllocator::PoolSizeRatio> _ratios, uint32_t maxSets)
{
    ratios = _ratios;
    setsPerPool = maxSets; //grow it next allocation
    pools.push_back(create_pool());
    initialised = true;
}

VkDescriptorPool DescriptorAllocator::create_pool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : ratios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * setsPerPool)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = setsPerPool;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    VkDescriptorPool pool;
    vkCreateDescriptorPool(ctx.device, &pool_info, nullptr, &pool);
    return pool;
}


void DescriptorAllocator::clear_pools()
{ 
    for (auto p : pools) {
        vkResetDescriptorPool(ctx.device, p, 0);
    }
    currentPool = 0;
}

void DescriptorAllocator::destroy_pools()
{
    for (auto p : pools) {
        vkDestroyDescriptorPool(ctx.device, p, nullptr);
    }
    currentPool = 0;
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout)
{
    assert(initialised);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext = VK_NULL_HANDLE;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pools[currentPool];
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(ctx.device, &allocInfo, &ds);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        currentPool++;
        if (currentPool == pools.size())
        {
            pools.push_back(create_pool());
            setsPerPool *= 1.5;
        }
        allocInfo.descriptorPool = pools[currentPool];
        VK_CHECK( vkAllocateDescriptorSets(ctx.device, &allocInfo, &ds));
    }
    return ds;
}
