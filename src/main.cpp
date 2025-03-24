#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "vk/core.hpp"
#include "vk/pipeline_builder.hpp"
#include "vk/internal.hpp"
#include "lib/util.hpp"
#include "vk/info.hpp"

VkDescriptorSetLayout computeImageDescLayout;
VkDescriptorSet computeImageDesc;
VkPipeline computePipeline;
VkPipeline vertexPipeline;
VkPipelineLayout computePipelineLayout;
VkPipelineLayout vertexPipelineLayout;
std::unordered_map<std::string, engine::MeshAsset> meshes;
using namespace engine;

float translate_z = 5.0;

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct VertexPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

void init_render_data()
{
    init();
    VkShaderModule gradient = create_shader_module("assets/shaders/gradient.comp");
    computeImageDescLayout = create_descriptor_set_layout({
                                 {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
                             }, VK_SHADER_STAGE_COMPUTE_BIT);
    computeImageDesc = ctx.descriptorAllocator.allocate(computeImageDescLayout);

    update_descriptor_sets(
        {
            {computeImageDesc, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
             VK_NULL_HANDLE, ctx.framebuffer.color[0].imageView, VK_IMAGE_LAYOUT_GENERAL}
        },
        {}
    );

    {
        ComputePipelineBuilder builder;
        computePipeline = builder
            .set_descriptor_set_layouts({computeImageDescLayout})
            .set_shader_module(gradient)
            .set_push_constant_ranges({ { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants)  } })
            .build();
        computePipelineLayout = builder.layout;
    }

    {
        GraphicsPipelineBuilder builder;
        vertexPipeline = builder
            .set_push_constant_ranges({ { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstants) } })
            .set_framebuffer(ctx.framebuffer)
            .set_shader_stages({
                {VK_SHADER_STAGE_VERTEX_BIT, create_shader_module("assets/shaders/colored_triangle_mesh.vert")},
                {VK_SHADER_STAGE_FRAGMENT_BIT, create_shader_module("assets/shaders/colored_triangle.frag")}
            })
            .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_viewport_state()
            .set_rasterization_state(VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
            .set_multisample_state() // disabled
            .set_depth_state(VK_COMPARE_OP_LESS_OR_EQUAL, true)
            .set_color_blend_states( {color_blend({VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
                                                  {VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO})} )
            .set_dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .build();

            vertexPipelineLayout = builder.layout;
    }

    load_gltf_meshes("assets/meshes/basicmesh.glb", meshes);
    assert(meshes["Sphere"].meshBuffers.indexBuffer.buffer != VK_NULL_HANDLE);

    clean_init();
}

void draw_imgui(VkImageView imageView)
{
    const auto& frame = get_frame();
    const VkCommandBuffer cmd = frame.commandBuffer;
	VkRenderingAttachmentInfo colorAttachment = info::color_attachment(imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = info::rendering(ctx.swapchain.extent, &colorAttachment, nullptr);
	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);
}
void draw_background()
{
    const auto& frame = get_frame();
    const VkCommandBuffer cmd = frame.commandBuffer;

    //draw gradient using compute shader
    ComputePushConstants data = {
        glm::vec4(1.0, 1.0, 0.0, 1.0),
        glm::vec4(0.0, 1.0, 1.0, 1.0),
        glm::vec4(0.0, 1.0, 0.0, 1.0),
        glm::vec4(1.0, 0.0, 0.0, 1.0),
    };

    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeImageDesc, 0, nullptr);
	vkCmdPushConstants(frame.commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(frame.commandBuffer, std::ceil(ctx.extent.width / 16.0), std::ceil(ctx.extent.height / 16.0), 1);
}

void draw_geometry()
{
    const auto& frame = get_frame();
    const VkCommandBuffer cmd = frame.commandBuffer;

    VkRenderingAttachmentInfo colorAttachment = info::color_attachment(ctx.framebuffer.color[0].imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = info::depth_attachment(ctx.framebuffer.depth.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo renderInfo = info::rendering(ctx.extent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipeline);

    //set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = ctx.extent.width;
	viewport.height = ctx.extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = viewport.width;
	scissor.extent.height = viewport.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);
    //vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipelineLayout, 0, 1, &imageSet, 0, nullptr);

	glm::mat4 view = glm::translate(glm::mat4(1.f), glm::vec3{ 0, 0, translate_z });
	// camera projection
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)ctx.extent.width / (float)ctx.extent.height, 0.1f, 10000.f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;

	VertexPushConstants push_constants;
	push_constants.worldMatrix = projection * view;
    MeshAsset& mesh = meshes["Suzanne"];
	push_constants.vertexBuffer = mesh.meshBuffers.vertexBufferAddress;

	vkCmdPushConstants(cmd, vertexPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, mesh.meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, mesh.surfaces.size() * 3, 1, mesh.surfaces[0].startIndex, 0, 0);
    vkCmdEndRendering(cmd);

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

    ctx.extent.width = std::min(ctx.screenExtent.width, ctx.swapchain.extent.width) * ctx.renderScale;
    ctx.extent.height = std::min(ctx.screenExtent.height, ctx.swapchain.extent.height) * ctx.renderScale;

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); 

    draw_background();
        
    image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    image_barrier(cmd, ctx.framebuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    draw_geometry();
    
    image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    blit(cmd, ctx.framebuffer.color[0].image, ctx.swapchain.images[swapchainImageIndex], ctx.extent, ctx.swapchain.extent);

    image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    draw_imgui(ctx.swapchain.imageViews[swapchainImageIndex]);
    image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("background")) {
			ImGui::SliderFloat("trans", &translate_z, -10.0, 10.0);
		}
		ImGui::End();

        ImGui::Render();
        render();
        glfwMakeContextCurrent(ctx.window);
        glfwSwapBuffers(ctx.window);
    }

    cleanup();
}
