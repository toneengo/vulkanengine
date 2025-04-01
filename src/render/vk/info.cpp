#include "info.hpp"
#include <vulkan/vulkan_core.h>

VkCommandPoolCreateInfo info::create::command_pool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext                   = nullptr;
    commandPoolInfo.flags                   = flags;
    commandPoolInfo.queueFamilyIndex        = queueFamilyIndex;
    return commandPoolInfo;
}

VkCommandBufferAllocateInfo info::allocate::command_buffer(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.pNext                       = nullptr;
    cmdAllocInfo.commandPool                 = pool;
    cmdAllocInfo.commandBufferCount          = count;
    cmdAllocInfo.level                       = level;
    return cmdAllocInfo;
}

//VkFramebufferCreateInfo framebuffer(VkRenderPass renderPass, VkExtent2D extent);
VkFenceCreateInfo info::create::fence(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext             = nullptr;
    info.flags             = flags;
    return info;
}

VkSemaphoreCreateInfo info::create::semaphore(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext                 = nullptr;
    info.flags                 = flags;
    return info;
}

VkImageCreateInfo info::create::image(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext             = nullptr;
    info.imageType         = VK_IMAGE_TYPE_2D;
    info.format            = format;
    info.extent            = extent;
    info.mipLevels         = 1;
    info.arrayLayers       = 1;
    info.samples           = VK_SAMPLE_COUNT_1_BIT; // no msaa
    info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    info.usage             = usageFlags;
    return info;
}

VkImageViewCreateInfo info::create::image_view(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info           = {};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext                           = nullptr;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.image                           = image;
    info.format                          = format;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;
    info.subresourceRange.aspectMask     = aspectFlags;
    return info;
}

VkComputePipelineCreateInfo info::create::compute_pipeline(VkPipelineLayout layout, VkPipelineShaderStageCreateInfo stage, VkPipelineCreateFlags flags) {
    VkComputePipelineCreateInfo info = {};
    info.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.pNext                       = nullptr;
    info.flags                       = flags;
    info.layout                      = layout;
    info.stage                       = stage;
    return info;
}

VkPipelineShaderStageCreateInfo pipeline_shader_stage(VkShaderStageFlagBits stage, VkShaderModule module) {
    return {.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = stage,
            .module              = module,
            .pName               = "main",
            .pSpecializationInfo = nullptr};
}

VkCommandBufferBeginInfo info::begin::command_buffer(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext                    = nullptr;
    info.pInheritanceInfo         = nullptr;
    info.flags                    = flags;
    return info;
}

//VkRenderPassBeginInfo renderpass_begin(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);
VkSemaphoreSubmitInfo info::submit::semaphore(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
    VkSemaphoreSubmitInfo submitInfo{};
    submitInfo.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submitInfo.pNext       = nullptr;
    submitInfo.semaphore   = semaphore;
    submitInfo.stageMask   = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value       = 1;
    return submitInfo;
}

VkCommandBufferSubmitInfo info::submit::command_buffer(VkCommandBuffer cmd) {
    VkCommandBufferSubmitInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext         = nullptr;
    info.commandBuffer = cmd;
    info.deviceMask    = 0;
    return info;
}

VkSubmitInfo2 info::submit::submit(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* waitSemaphoreInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo) {
    VkSubmitInfo2 info            = {};
    info.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext                    = nullptr;
    info.waitSemaphoreInfoCount   = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos      = waitSemaphoreInfo;
    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos    = signalSemaphoreInfo;
    info.commandBufferInfoCount   = 1;
    info.pCommandBufferInfos      = cmd;
    return info;
}

//just one swapchain and semaphore
VkPresentInfoKHR info::present(VkSwapchainKHR* swapchain, VkSemaphore* waitSemaphore, uint32_t* imageIndex) {
    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.pSwapchains        = swapchain;
    presentInfo.swapchainCount     = 1;
    presentInfo.pWaitSemaphores    = waitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = imageIndex;
    return presentInfo;
}

VkRenderingAttachmentInfo info::color_attachment(VkImageView view, VkClearValue* clear, VkImageLayout layout) {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext       = nullptr;
    colorAttachment.imageView   = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear) {
        colorAttachment.clearValue = *clear;
    }
    return colorAttachment;
}

VkRenderingAttachmentInfo info::depth_attachment(VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/) {
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType                         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.pNext                         = nullptr;
    depthAttachment.imageView                     = view;
    depthAttachment.imageLayout                   = layout;
    depthAttachment.loadOp                        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                       = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth = 1.f;
    return depthAttachment;
}

VkRenderingInfo info::rendering(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment, VkRenderingFlags flags) {
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.pNext                = nullptr;
    renderingInfo.flags                = flags;
    renderingInfo.renderArea           = {0, 0, extent.width, extent.height};
    renderingInfo.layerCount           = 1;
    renderingInfo.viewMask             = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments    = colorAttachment;
    renderingInfo.pDepthAttachment     = depthAttachment;
    return renderingInfo;
}
