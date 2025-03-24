#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "types.hpp"

namespace engine
{
    //utility functions
    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);
    void image_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT);

    void blit(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
    VkShaderModule create_shader_module(const char* filePath);
    void destroy_shader_module(VkShaderModule module);

    void init();
    void cleanup();
    VkDescriptorSetLayout create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags = 0);
    Image create_image_from_pixels(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    Image create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    Image create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    Buffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void create_swapchain(uint32_t width, uint32_t height);

    //default pipelines, renders triangles
    //vertex -> fragment pipeline
    VkPipeline create_default_graphics_pipeline(const char* vertPath, const char* fragPath);
    //compute pipeline
    /*
    VkPipeline create_compute_pipeline(const char* compPath);
    //can make your own pipeline create info
    VkPipeline create_custom_graphics_pipeline(VkGraphicsPipelineCreateInfo info);
    VkPipeline create_custom_compute_pipeline(VkComputePipelineCreateInfo info);
    */

    void destroy_image(Image image);
    void destroy_buffer(Buffer buffer);

    VkCommandBuffer get_immediate_command_buffer();
    void begin_immediate_command();
    void end_immediate_command();

    //raii version of above commands
    struct ImmediateCommandGuard
    {
        ImmediateCommandGuard();
        ~ImmediateCommandGuard();
    };

    void clean();
}
