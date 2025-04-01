#include "destroy.hpp"
#include "internal.hpp"
#include <cstdio>

void spock::Object::destroy() {
#ifdef DBG
    if (lineNumber != -1)
        printf("Destroying object at %s:%d\n", fileName, lineNumber);
#endif
    switch (type) {
        case OBJ::Image: vmaDestroyImage(ctx.allocator, image, allocation); break;
        case OBJ::ImageView: vkDestroyImageView(ctx.device, imageView, nullptr); break;
        case OBJ::Allocator: vmaDestroyAllocator(ctx.allocator); break;
        case OBJ::DescriptorPool: vkDestroyDescriptorPool(ctx.device, dp, nullptr); break;
        case OBJ::DescriptorSetLayout: vkDestroyDescriptorSetLayout(ctx.device, dsl, nullptr); break;
        case OBJ::PipelineLayout: vkDestroyPipelineLayout(ctx.device, pll, nullptr); break;
        case OBJ::Pipeline: vkDestroyPipeline(ctx.device, pl, nullptr); break;
        case OBJ::Fence: vkDestroyFence(ctx.device, fence, nullptr); break;
        case OBJ::CommandPool: vkDestroyCommandPool(ctx.device, commandPool, nullptr); break;
        case OBJ::Buffer: vmaDestroyBuffer(ctx.allocator, buffer, allocation); break;
        case OBJ::Sampler: vkDestroySampler(ctx.device, sampler, nullptr); break;
        default: break;
    }
}

void spock::DestroyQueue::push(Object obj) {
    queue.push_back(obj);
}

void spock::DestroyQueue::flush() {
    for (auto it = queue.rbegin(); it != queue.rend(); it++) {
        it->destroy();
    }
    queue.clear();
}
