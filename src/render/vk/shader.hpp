#pragma once
#include <vulkan/vulkan_core.h>
namespace spock {
    VkShaderModule create_shader_module(const char* filePath);
    void           destroy_shader_module(VkShaderModule module);
    void           clean_shader_modules();
}
