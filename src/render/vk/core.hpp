#pragma once
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "types.hpp"
#include "shader.hpp"

namespace spock {
    void  clean_init();
    Model load_gltf_model(const char* filePath);

    struct ImageWrite {
        VkDescriptorSet  descriptorSet;
        uint32_t         binding;
        VkDescriptorType descriptorType;
        VkSampler        sampler;
        VkImageView      imageView;
        VkImageLayout    imageLayout;
        uint32_t         index = 0;
        uint32_t         count = 1;
    };
    struct BufferWrite {
        VkDescriptorSet  descriptorSet;
        uint32_t         binding;
        VkDescriptorType descriptorType;
        VkBuffer         buffer;
        VkDeviceSize     offset;
        VkDeviceSize     range;
        uint32_t         index = 0;
        uint32_t         count = 1;
    };

    void                  update_descriptor_sets(std::initializer_list<ImageWrite> imageWrites, std::initializer_list<BufferWrite> bufferWrites);

    void                  init();
    void                  init_imgui();
    void                  draw_imgui(VkImageView imageView);
    void                  cleanup();
    VkDescriptorSetLayout create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags = 0);
    Image                 create_image_from_pixels(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    Image                 create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    Image                 create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    Image                 create_texture(const char* fileName, VkDescriptorSet descriptorSet, uint32_t binding, VkSampler sampler, VkImageUsageFlags usage, bool mipmapped = false);
    Buffer                create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void                  create_swapchain(uint32_t width, uint32_t height);

    
    void                  set_scissor(); 

    void                  destroy_image(Image image);
    void                  destroy_buffer(Buffer buffer);

    VkCommandBuffer       get_immediate_command_buffer();
    void                  begin_immediate_command();
    void                  end_immediate_command();

    void*                 get_mapped_data(VmaAllocation a);

    void                  clean();
}
