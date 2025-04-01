#include <vulkan/vulkan_core.h>
#include <initializer_list>

namespace info {
    namespace create {
        VkCommandPoolCreateInfo command_pool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
        //unused in dynamic rendering
        //VkFramebufferCreateInfo framebuffer(VkRenderPass renderPass, VkExtent2D extent);
        VkFenceCreateInfo               fence(VkFenceCreateFlags flags = 0);
        VkSemaphoreCreateInfo           semaphore(VkSemaphoreCreateFlags flags = 0);
        VkImageCreateInfo               image(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
        VkImageViewCreateInfo           image_view(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
        VkPipelineLayoutCreateInfo      pipeline_layout(std::initializer_list<VkDescriptorSetLayout> descriptorSetLayouts,
                                                        std::initializer_list<VkPushConstantRange>   pushConstantRanges);
        VkComputePipelineCreateInfo     compute_pipeline(VkPipelineLayout layout, VkPipelineShaderStageCreateInfo stage, VkPipelineCreateFlags flags = 0);
        VkPipelineShaderStageCreateInfo pipeline_shader_stage(VkShaderStageFlagBits stage, VkShaderModule module);
    }

    namespace allocate {
        VkCommandBufferAllocateInfo command_buffer(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }

    namespace begin {
        VkCommandBufferBeginInfo command_buffer(VkCommandBufferUsageFlags flags = 0);
        //not used in dynamic rendering
        //VkRenderPassBeginInfo renderpass_begin(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);
    }

    namespace submit {
        VkSemaphoreSubmitInfo     semaphore(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
        VkCommandBufferSubmitInfo command_buffer(VkCommandBuffer cmd);
        VkSubmitInfo2             submit(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
    }

    VkPresentInfoKHR          present(VkSwapchainKHR* swapchain, VkSemaphore* waitSemaphore, uint32_t* imageIndex);
    VkRenderingInfo           rendering(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment, VkRenderingFlags flags = 0);
    VkRenderingAttachmentInfo color_attachment(VkImageView view, VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);
    VkRenderingAttachmentInfo depth_attachment(VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);
}

void pipeline_init_clean();
