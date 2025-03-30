#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "lib/util.hpp"
#include "vk/core.hpp"
#include "vk/info.hpp"
#include "vk/internal.hpp"
#include "vk/pipeline_builder.hpp"

VkDescriptorSetLayout computeImageDescLayout;
VkDescriptorSetLayout uniformDescLayout;
VkDescriptorSet computeImageDesc;
VkPipeline computePipeline;
VkPipeline vertexPipeline;
VkPipelineLayout computePipelineLayout;
VkPipelineLayout vertexPipelineLayout;

using namespace engine;
Model guitar;

// Select a binding for each descriptor type
constexpr int STORAGE_BINDING = 0;
constexpr int SAMPLER_BINDING = 1;
constexpr int IMAGE_BINDING = 2;
// Max count of each descriptor type
// You can query the max values for these with
// physicalDevice.getProperties().limits.maxDescriptrorSet*******
float translate_z = 5.0;

struct ComputePushConstants {
  glm::vec4 data1;
  glm::vec4 data2;
  glm::vec4 data3;
  glm::vec4 data4;
};

struct VertexPushConstants {
  int diffuse;
  int normal;
  int specular;
  glm::mat4 worldMatrix;
  VkDeviceAddress vertexBuffer;
};

struct GPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
};

GPUSceneData camera = {};

struct Input {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
} input;

float speed = 0.1;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_W && action == GLFW_PRESS)
    input.forward = true;
  else if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    input.forward = false;

  if (key == GLFW_KEY_A && action == GLFW_PRESS)
    input.left = true;
  else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    input.left = false;

  if (key == GLFW_KEY_S && action == GLFW_PRESS)
    input.backward = true;
  else if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    input.backward = false;

  if (key == GLFW_KEY_D && action == GLFW_PRESS)
    input.right = true;
  else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    input.right = false;

  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    input.up = true;
  else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
    input.up = false;

  if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
    input.down = true;
  else if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
    input.down = false;
}

void update_camera() {
  float h = ((input.forward || input.backward) && (input.left || input.right))
                ? sqrt(speed * speed / 2.f)
                : speed;
  float vertical = 0 + input.down * h - input.up * h;
  float horizontal = 0 + input.left * h - input.right * h;
  float depth = 0 + input.forward * h - input.backward * h;

  camera.view =
      glm::translate(camera.view, glm::vec3(horizontal, vertical, depth));
}

void init_render_data() {
  init();
  VkShaderModule gradient =
      create_shader_module("assets/shaders/gradient.comp");
  computeImageDescLayout = create_descriptor_set_layout(
      {{0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}}, VK_SHADER_STAGE_COMPUTE_BIT);
  uniformDescLayout = create_descriptor_set_layout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}},
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  computeImageDesc = ctx.descriptorAllocator.allocate(computeImageDescLayout);

  update_descriptor_sets(
      {{computeImageDesc, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE,
        ctx.framebuffer.color[0].imageView, VK_IMAGE_LAYOUT_GENERAL}},
      {});

  {
    ComputePipelineBuilder builder;
    computePipeline =
        builder.set_descriptor_set_layouts({computeImageDescLayout})
            .set_shader_module(gradient)
            .set_push_constant_ranges({{VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                        sizeof(ComputePushConstants)}})
            .build();
    computePipelineLayout = builder.layout;
  }

  {
    GraphicsPipelineBuilder builder;
    vertexPipeline =
        builder
            .set_descriptor_set_layouts({uniformDescLayout, samplerDescriptorSetLayout})
            .set_push_constant_ranges(
                {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstants)}})
            .set_framebuffer(ctx.framebuffer)
            .set_shader_stages(
                {{VK_SHADER_STAGE_VERTEX_BIT,
                  create_shader_module(
                      "assets/shaders/colored_triangle_mesh.vert")},
                 {VK_SHADER_STAGE_FRAGMENT_BIT,
                  create_shader_module(
                      "assets/shaders/colored_triangle.frag")}})
            .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_viewport_state()
            .set_rasterization_state(VK_POLYGON_MODE_FILL, 1.f,
                                     VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
            .set_multisample_state() // disabled
            .set_depth_state(VK_COMPARE_OP_LESS_OR_EQUAL, true)
            .set_color_blend_states(
                {color_blend({VK_BLEND_FACTOR_SRC_ALPHA,
                              VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
                             {VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO})})
            .set_dynamic_states(
                {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .build();

    vertexPipelineLayout = builder.layout;
  }

  guitar = load_gltf_model("assets/meshes/guitar/backpack.obj");

  clean_init();
}

void draw_imgui(VkImageView imageView) {
  const auto &frame = get_frame();
  const VkCommandBuffer cmd = frame.commandBuffer;
  VkRenderingAttachmentInfo colorAttachment = info::color_attachment(
      imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingInfo renderInfo =
      info::rendering(ctx.swapchain.extent, &colorAttachment, nullptr);
  vkCmdBeginRendering(cmd, &renderInfo);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  vkCmdEndRendering(cmd);
}

void draw_background() {
  const auto &frame = get_frame();
  const VkCommandBuffer cmd = frame.commandBuffer;

  // draw gradient using compute shader
  ComputePushConstants data = {
      glm::vec4(1.0, 1.0, 0.0, 1.0),
      glm::vec4(0.0, 1.0, 1.0, 1.0),
      glm::vec4(0.0, 1.0, 0.0, 1.0),
      glm::vec4(1.0, 0.0, 0.0, 1.0),
  };

  vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    computePipeline);
  vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          computePipelineLayout, 0, 1, &computeImageDesc, 0,
                          nullptr);
  vkCmdPushConstants(frame.commandBuffer, computePipelineLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ComputePushConstants), &data);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(frame.commandBuffer, std::ceil(ctx.extent.width / 16.0),
                std::ceil(ctx.extent.height / 16.0), 1);
}

void draw_geometry() {
  auto &frame = get_frame();
  const VkCommandBuffer cmd = frame.commandBuffer;

  VkRenderingAttachmentInfo colorAttachment =
      info::color_attachment(ctx.framebuffer.color[0].imageView, nullptr,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingAttachmentInfo depthAttachment =
      info::depth_attachment(ctx.framebuffer.depth.imageView,
                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  VkRenderingInfo renderInfo =
      info::rendering(ctx.extent, &colorAttachment, &depthAttachment);
  vkCmdBeginRendering(cmd, &renderInfo);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipeline);

  // set dynamic viewport and scissor
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
  // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
  // vertexPipelineLayout, 0, 1, &imageSet, 0, nullptr);
  Buffer gpuSceneDataBuffer =
      create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);
  frame.destroyQueue.push(gpuSceneDataBuffer);

  // write the buffer
  GPUSceneData *sceneUniformData =
      (GPUSceneData *)get_mapped_data(gpuSceneDataBuffer.allocation);
  *sceneUniformData = camera;

  // create a descriptor set that binds that buffer and update it
  VkDescriptorSet globalDescriptor =
      frame.descriptorAllocator.allocate(uniformDescLayout);

  update_descriptor_sets(
      {}, {{globalDescriptor, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            gpuSceneDataBuffer.buffer, 0, sizeof(GPUSceneData)}});

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vertexPipelineLayout, 0, 1, &globalDescriptor, 0,
                          nullptr);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vertexPipelineLayout, 1, 1, &samplerDescriptorSet, 0,
                          nullptr);

  VertexPushConstants push_constants;
  // push_constants.worldMatrix = projection * view;
  /*
  MeshAsset &mesh = meshes["Suzanne"];
  push_constants.vertexBuffer = mesh.meshBuffers.vertexBufferAddress;

  vkCmdPushConstants(cmd, vertexPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(VertexPushConstants), &push_constants);
  vkCmdBindIndexBuffer(cmd, mesh.meshBuffers.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(cmd, mesh.surfaces.size() * 3, 1,
                   mesh.surfaces[0].startIndex, 0, 0);
                   */

  for (const auto& mesh : guitar.meshes)
  {
      push_constants.diffuse = mesh.diffuse.index;
      push_constants.normal = mesh.normal.index;
      push_constants.specular = mesh.specular.index;
      push_constants.vertexBuffer = mesh.meshBuffers.vertexBufferAddress;
      push_constants.worldMatrix = glm::mat4(1.0); //#TODO: make this usable

      vkCmdPushConstants(cmd, vertexPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                         sizeof(VertexPushConstants), &push_constants);
      vkCmdBindIndexBuffer(cmd, mesh.meshBuffers.indexBuffer.buffer, 0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdDrawIndexed(cmd, mesh.surfaces.size() * 3, 1,
                       mesh.surfaces[0].startIndex, 0, 0);

  }
  vkCmdEndRendering(cmd);
}

void render() {
  auto &frame = get_frame();
  VK_CHECK(
      vkWaitForFences(ctx.device, 1, &frame.renderFence, true, 1000000000));
  frame.destroyQueue.flush();
  frame.descriptorAllocator.clear_pools();
  VK_CHECK(vkResetFences(ctx.device, 1, &frame.renderFence));

  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(ctx.device, ctx.swapchain.swapchain,
                                 1000000000, frame.swapchainSemaphore, nullptr,
                                 &swapchainImageIndex));

  VkCommandBuffer cmd = frame.commandBuffer;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));
  VkCommandBufferBeginInfo cmdBeginInfo =
      info::begin::command_buffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  ctx.extent.width =
      std::min(ctx.screenExtent.width, ctx.swapchain.extent.width) *
      ctx.renderScale;
  ctx.extent.height =
      std::min(ctx.screenExtent.height, ctx.swapchain.extent.height) *
      ctx.renderScale;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL);

  draw_background();

  image_barrier(cmd, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  image_barrier(cmd, ctx.framebuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  // camera projection
  camera.proj = glm::perspective(
      glm::radians(70.f), (float)ctx.extent.width / (float)ctx.extent.height,
      0.1f, 10000.f);
  // invert the Y direction on projection matrix so that we are more similar
  // to opengl and gltf axis
  camera.proj[1][1] *= -1;
  update_camera();
  draw_geometry();

  image_barrier(cmd, ctx.framebuffer.color[0].image,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex],
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  blit(cmd, ctx.framebuffer.color[0].image,
       ctx.swapchain.images[swapchainImageIndex], ctx.extent,
       ctx.swapchain.extent);

  image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex],
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  draw_imgui(ctx.swapchain.imageViews[swapchainImageIndex]);
  image_barrier(cmd, ctx.swapchain.images[swapchainImageIndex],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdInfo = info::submit::command_buffer(cmd);
  VkSemaphoreSubmitInfo waitInfo = info::submit::semaphore(
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      frame.swapchainSemaphore);
  VkSemaphoreSubmitInfo signalInfo = info::submit::semaphore(
      VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.renderSemaphore);
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

int main() {
  camera.view = glm::translate(glm::mat4(1.f), glm::vec3{0, 0, -5});
  init_render_data();

  glfwSetKeyCallback(ctx.window, key_callback);

  while (!glfwWindowShouldClose(ctx.window)) {
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
