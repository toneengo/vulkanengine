#pragma once
#include <vulkan/vulkan_core.h>

namespace spock {
    inline VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask) {
        VkImageSubresourceRange subImage{};
        subImage.aspectMask     = aspectMask;
        subImage.baseMipLevel   = 0;
        subImage.levelCount     = VK_REMAINING_MIP_LEVELS;
        subImage.baseArrayLayer = 0;
        subImage.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        return subImage;
    }

    struct BarrierMask {
        VkPipelineStageFlags2 srcStageMask;
        VkAccessFlags2        srcAccessMask;
        VkPipelineStageFlags2 dstStageMask;
        VkAccessFlags2        dstAccessMask;
    };

    inline void image_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
                              VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                              VkPipelineStageFlags2 dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                              VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT) {
        VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        imageBarrier.pNext = nullptr;

        imageBarrier.srcStageMask  = srcStageMask;
        imageBarrier.srcAccessMask = srcAccessMask;
        imageBarrier.dstStageMask  = dstStageMask;
        imageBarrier.dstAccessMask = dstAccessMask;

        imageBarrier.oldLayout = currentLayout;
        imageBarrier.newLayout = newLayout;

        VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = image_subresource_range(aspectMask);
        imageBarrier.image            = image;

        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;

        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers    = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    inline void blit(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize) {
        VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

        blitRegion.srcOffsets[1].x = srcSize.width;
        blitRegion.srcOffsets[1].y = srcSize.height;
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstSize.width;
        blitRegion.dstOffsets[1].y = dstSize.height;
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount     = 1;
        blitRegion.srcSubresource.mipLevel       = 0;

        blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount     = 1;
        blitRegion.dstSubresource.mipLevel       = 0;

        VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
        blitInfo.dstImage       = dst;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage       = src;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter         = VK_FILTER_LINEAR;
        blitInfo.regionCount    = 1;
        blitInfo.pRegions       = &blitRegion;

        vkCmdBlitImage2(cmd, &blitInfo);
    }
    inline void           set_viewport(const VkCommandBuffer& cmd, float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f)
    {
        VkViewport viewport = {};
        viewport.x          = x;
        viewport.y          = y;
        viewport.width      = width;
        viewport.height     = height;
        viewport.minDepth   = minDepth;
        viewport.maxDepth   = maxDepth;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
    } 
    
    inline void set_scissor(const VkCommandBuffer& cmd, float x, float y, float width, float height)
    {
        VkRect2D scissor      = {};
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = width;
        scissor.extent.height = height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

}

#define write_uniform_buffer_descriptor(ds, src, size,dq) do {\
    Buffer buf = create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);\
    dq.push(buf);\
    void* dst = get_mapped_data(buf.allocation);\
    memcpy(dst, src, size);\
    update_descriptor_sets({}, {{ds, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buf.buffer, 0, size}});\
} while(0)

