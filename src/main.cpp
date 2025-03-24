#include "vk/core.hpp"
#include "vk/pipeline_builder.hpp"
#include "vk/internal.hpp"
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include "lib/util.hpp"
#include "vk/info.hpp"

VkDescriptorSetLayout computeImageDescLayout;
VkDescriptorSet computeImageDesc;
VkPipeline computePipeline;
VkPipelineLayout computePipelineLayout;
using namespace engine;

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

void init_render_data()
{
    init();
    VkShaderModule gradient = create_shader_module("assets/shaders/gradient_color.comp");
    computeImageDescLayout = create_descriptor_set_layout({
                                 {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
                             }, VK_SHADER_STAGE_COMPUTE_BIT);
    computeImageDesc = ctx.descriptorAllocator.allocate(computeImageDescLayout);

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = ctx.framebuffer.color[0].imageView;

    VkWriteDescriptorSet drawImageWrite = {};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = computeImageDesc;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(ctx.device, 1, &drawImageWrite, 0, nullptr);
    ComputePipelineBuilder builder;
    computePipeline = builder
        .set_descriptor_set_layouts({computeImageDescLayout})
        .set_shader_module(gradient)
        .set_push_constant_ranges({ { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants)  } })
        .build();
    computePipelineLayout = builder.layout;
    vkDestroyShaderModule(ctx.device, gradient, nullptr);
}

void render()
{
    auto& frame = get_frame();
    VK_CHECK(vkWaitForFences(ctx.device, 1, &frame.renderFence, true, 1000000000));
    frame.destroyQueue.flush();
    frame.descriptorAllocator.clear_pools();
	VK_CHECK(vkResetFences(ctx.device, 1, &frame.renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(ctx.device, ctx.swapchain.swapchain, 1000000000, frame.swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = frame.commandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = info::begin::command_buffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkExtent2D extent;
    extent.width = std::min(ctx.screenExtent.width, ctx.screenExtent.width) * ctx.renderScale;
    extent.height = std::min(ctx.screenExtent.height, ctx.screenExtent.height) * ctx.renderScale;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); 

    //draw gradient using compute shader
    ComputePushConstants data = {
        glm::vec4(1.0, 1.0, 0.0, 1.0),
        glm::vec4(0.0, 1.0, 1.0, 1.0),
        glm::vec4(0.0, 1.0, 0.0, 1.0),
        glm::vec4(1.0, 0.0, 0.0, 1.0),
    };

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeImageDesc, 0, nullptr);
	vkCmdPushConstants(cmd, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);

    image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    blit(cmd, ctx.framebuffer.color[0].image, ctx.swapchain.images[swapchainImageIndex], extent, ctx.swapchain.extent);
    image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = info::submit::command_buffer(cmd);
    VkSemaphoreSubmitInfo waitInfo = info::submit::semaphore(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = info::submit::semaphore(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.renderSemaphore);
    VkSubmitInfo2 submit = info::submit::submit(&cmdInfo, &waitInfo, &signalInfo);
	VK_CHECK(vkQueueSubmit2(ctx.graphicsQueue, 1, &submit, frame.renderFence));

    VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &ctx.swapchain.swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &frame.renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;
	VK_CHECK(vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo));

	ctx.frameIdx++;
}

int main()
{
    init_render_data();

    while (!glfwWindowShouldClose(ctx.window))
    {
        glfwPollEvents();
        render();
        glfwMakeContextCurrent(ctx.window);
        glfwSwapBuffers(ctx.window);
    }

    cleanup();
}
