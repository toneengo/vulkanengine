#include "del_queue.hpp"
#include "context.hpp"
void Object::destroy()
{
    switch (type)
    {
        case VKOBJ::AllocatedImage:
            vkDestroyImageView(ctx->device, image.imageView, nullptr);
            vmaDestroyImage(ctx->allocator, image.image, image.allocation);
            break;
        case VKOBJ::Allocator:
            vmaDestroyAllocator(ctx->allocator);
            break;
        case VKOBJ::DescriptorPool:
            vkDestroyDescriptorPool(ctx->device, dp, nullptr);
            break;
        case VKOBJ::DescriptorSetLayout:
            vkDestroyDescriptorSetLayout(ctx->device, dsl, nullptr); 
            break;
        case VKOBJ::PipelineLayout:
            vkDestroyPipelineLayout(ctx->device, pll, nullptr);
            break;
        case VKOBJ::Pipeline:
            vkDestroyPipeline(ctx->device, pl, nullptr);
            break;
        case VKOBJ::Fence:
            vkDestroyFence(ctx->device, fence, nullptr);
            break;
        case VKOBJ::CommandPool:
            vkDestroyCommandPool(ctx->device, commandPool, nullptr);
            break;
        case VKOBJ::AllocatedBuffer:
            vmaDestroyBuffer(ctx->allocator, buffer.buffer, buffer.allocation);
            break;
        default:
            break;
    }
}

void DeletionQueue::flush(VkDevice _device, VmaAllocator _allocator)
{
    for (int i = queue.size; i >= 0; i--)
    {
        queue[i].destroy();
    }
    queue.clear();
}

void DeletionQueue::push(Object _o)
{
    _o.ctx = ctx;
    queue.push_back(_o);
}
