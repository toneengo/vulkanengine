#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>
#include <vk/context.hpp>
#include <cstring>
#include "lib/ds.hpp"

VulkanContext ctx;

uint32_t SCR_WIDTH = 800;
uint32_t SCR_HEIGHT = 600;

void init_glfw_window()
{
}

int main()
{
    init_glfw_window();

    ctx.init();

    ctx.render_loop();

    ctx.cleanup();

    return 0;
}
