#pragma once
#include <vulkan/vulkan.hpp>
namespace vkutil {
    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);
    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
    void blit_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
    bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}
