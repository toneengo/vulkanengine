//All the commands used to render the scene are called here.
//this is vulkan "user" code, hence why it is separate from core.hpp
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "lib/util.hpp"
#include "vk/core.hpp"
#include "vk/info.hpp"
#include "vk/internal.hpp"
#include "vk/pipeline_builder.hpp"
#include "vk/util.hpp"
#include "input.hpp"
#include "render.hpp"

#include "render_data.hpp"

using namespace spock;
struct {
    double yaw = 90.f;
    double pitch = 0.f;
    double roll = 0.f;
    double sensitivity = 0.1f;
    double speed = 0.05f;

    glm::dvec3 pos = {0, 0, -5};
    glm::dvec3 front = {0, 0, 1};
    glm::dvec3 up = {0, 1, 0};
    glm::dvec3 right = {1, 0, 0};

    void update()
    {
        double msPassed = double(delta.count()) / double(NS_PER_MS);
        right = glm::normalize(glm::cross({0, 1, 0}, front));
        up = glm::cross(front, right);
        if (input.mouseStates[GLFW_MOUSE_BUTTON_2] == KEY_Held)
        {
            yaw += input.mouseRelativeMotion.x * sensitivity;
            pitch += input.mouseRelativeMotion.y * sensitivity;
        }

        if(pitch > 89.0f)
            pitch = 89.0f;
        if(pitch < -89.0f)
            pitch = -89.0f;
        glm::vec3 direction;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);

        if (input.keyStates[GLFW_KEY_W] & KEY_PressORHeld) pos += speed * front * msPassed;
        if (input.keyStates[GLFW_KEY_A] & KEY_PressORHeld) pos += speed * right * msPassed;
        if (input.keyStates[GLFW_KEY_S] & KEY_PressORHeld) pos -= speed * front * msPassed;
        if (input.keyStates[GLFW_KEY_D] & KEY_PressORHeld) pos -= speed * right * msPassed;
        if (input.keyStates[GLFW_KEY_SPACE] & KEY_PressORHeld) pos += speed * glm::dvec3{0,1,0} * msPassed;
        if (input.keyStates[GLFW_KEY_LEFT_CONTROL] & KEY_PressORHeld) pos -= speed * glm::dvec3{0,1,0} * msPassed;
    }

    glm::mat4 view_matrix()
    {
        return glm::lookAt(pos, pos + front, up);
    }
} camera;

void spock::init_engine() {
    init();

    //initialise descriptor allocators
    ctx.descriptorAllocator.set_flags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    ctx.descriptorAllocator.init(
        {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMAGE_COUNT}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SAMPLER_COUNT}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, STORAGE_COUNT}}, 1);
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        ctx.frames[i].descriptorAllocator.init(
            {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}},
            1000);
    }

    //create default samplers
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampl.magFilter           = VK_FILTER_NEAREST;
    sampl.minFilter           = VK_FILTER_NEAREST;
    vkCreateSampler(ctx.device, &sampl, nullptr, &nearestSampler);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(ctx.device, &sampl, nullptr, &linearSampler);

    QUEUE_DESTROY_OBJ(linearSampler);
    QUEUE_DESTROY_OBJ(nearestSampler);

    //create descriptor set layouts
    samplerDescriptorSetLayout = create_descriptor_set_layout(
        {{SAMPLER_BINDING, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SAMPLER_COUNT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT}},
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

    samplerDescriptorSet = ctx.descriptorAllocator.allocate(samplerDescriptorSetLayout);
    //let the global  vulkan context know where the texture descriptor set is

    init_input_callbacks();
    init_imgui();

    VkShaderModule gradient = create_shader_module("assets/shaders/gradient.comp");
    computeImageDescLayout  = create_descriptor_set_layout({{0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}}, VK_SHADER_STAGE_COMPUTE_BIT);
    uniformDescLayout       = create_descriptor_set_layout({{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}}, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    computeImageDesc        = ctx.descriptorAllocator.allocate(computeImageDescLayout);

    update_descriptor_sets({{computeImageDesc, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, ctx.framebuffer.color[0].imageView, VK_IMAGE_LAYOUT_GENERAL}}, {});

    {
        ComputePipelineBuilder builder;
        computePipeline = builder.set_descriptor_set_layouts({computeImageDescLayout})
                              .set_shader_module(gradient)
                              .set_push_constant_ranges({{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants)}})
                              .build();
        computePipelineLayout = builder.layout;
    }

    {
        GraphicsPipelineBuilder builder;
        vertexPipeline = builder.set_descriptor_set_layouts({uniformDescLayout, samplerDescriptorSetLayout})
                             .set_push_constant_ranges({{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstants)}})
                             .set_framebuffer(ctx.framebuffer)
                             .set_shader_stages({{VK_SHADER_STAGE_VERTEX_BIT, create_shader_module("assets/shaders/mesh.vert")},
                                                 {VK_SHADER_STAGE_FRAGMENT_BIT, create_shader_module("assets/shaders/mesh.frag")}})
                             .set_input_assembly_state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                             .set_viewport_state()
                             .set_rasterization_state(VK_POLYGON_MODE_FILL, 1.f, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                             .set_multisample_state() // disabled
                             .set_depth_state(VK_COMPARE_OP_LESS_OR_EQUAL, true)
                             .set_color_blend_states({color_blend({VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA}, {VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO})})
                             .set_dynamic_states({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
                             .build();

        vertexPipelineLayout = builder.layout;
    }

    guitar = load_gltf_model("assets/meshes/guitar/backpack.obj");

    clean_init();
}

uint32_t swapchainImageIndex = 0;
static FrameContext* frame = nullptr;
void draw_background() {
    // draw gradient using compute shader
    ComputePushConstants data = {
        glm::vec4(1.0, 1.0, 0.0, 1.0),
        glm::vec4(0.0, 1.0, 1.0, 1.0),
        glm::vec4(0.0, 1.0, 0.0, 1.0),
        glm::vec4(1.0, 0.0, 0.0, 1.0),
    };

    vkCmdBindPipeline(frame->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(frame->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeImageDesc, 0, nullptr);
    vkCmdPushConstants(frame->commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &data);

    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
    // we need to divide by it
    vkCmdDispatch(frame->commandBuffer, std::ceil(ctx.extent.width / 16.0), std::ceil(ctx.extent.height / 16.0), 1);
}

void draw_geometry() {

    VkRenderingAttachmentInfo colorAttachment = info::color_attachment(ctx.framebuffer.color[0].imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = info::depth_attachment(ctx.framebuffer.depth.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo           renderInfo = info::rendering(ctx.extent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(frame->commandBuffer, &renderInfo);

    vkCmdBindPipeline(frame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipeline);

    // set dynamic viewport and scissor
    set_viewport(frame->commandBuffer, 0, 0, ctx.extent.width, ctx.extent.height);
    set_scissor(frame->commandBuffer, 0, 0, ctx.extent.width, ctx.extent.height);

    VkDescriptorSet globalDescriptor = frame->descriptorAllocator.allocate(uniformDescLayout);

    write_uniform_buffer_descriptor(globalDescriptor, &sceneData, sizeof(GPUSceneData));

    vkCmdBindDescriptorSets(frame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);
    vkCmdBindDescriptorSets(frame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipelineLayout, SAMPLER_BINDING, 1, &samplerDescriptorSet, 0, nullptr);

    VertexPushConstants push_constants;

    for (const auto& mesh : guitar.meshes) {
        push_constants.diffuse      = mesh.diffuse.index;
        push_constants.normal       = mesh.normal.index;
        push_constants.specular     = mesh.specular.index;
        push_constants.vertexBuffer = mesh.data.vertexBufferAddress;
        push_constants.worldMatrix  = glm::mat4(1.0); //#TODO: make this usable

        vkCmdPushConstants(frame->commandBuffer, vertexPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstants), &push_constants);
        vkCmdBindIndexBuffer(frame->commandBuffer, mesh.data.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(frame->commandBuffer, mesh.data.indexCount, 1, mesh.data.startIndex, 0, 0);
    }
    vkCmdEndRendering(frame->commandBuffer);
}

static void new_frame() {
    frame = &get_frame();
    VK_CHECK(vkWaitForFences(ctx.device, 1, &frame->renderFence, true, 1000000000));
    frame->destroyQueue.flush();
    frame->descriptorAllocator.clear_pools();
    VK_CHECK(vkResetFences(ctx.device, 1, &frame->renderFence));

    VK_CHECK(vkAcquireNextImageKHR(ctx.device, ctx.swapchain.swapchain, 1000000000, frame->swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = frame->commandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = info::begin::command_buffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    ctx.extent.width  = std::min(ctx.screenExtent.width, ctx.swapchain.extent.width) * ctx.renderScale;
    ctx.extent.height = std::min(ctx.screenExtent.height, ctx.swapchain.extent.height) * ctx.renderScale;

    VK_CHECK(vkBeginCommandBuffer(frame->commandBuffer, &cmdBeginInfo));
}

static void end_frame() {
    VK_CHECK(vkEndCommandBuffer(frame->commandBuffer));

    VkCommandBufferSubmitInfo cmdInfo    = info::submit::command_buffer(frame->commandBuffer);
    VkSemaphoreSubmitInfo     waitInfo   = info::submit::semaphore(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame->swapchainSemaphore);
    VkSemaphoreSubmitInfo     signalInfo = info::submit::semaphore(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame->renderSemaphore);
    VkSubmitInfo2             submit     = info::submit::submit(&cmdInfo, &waitInfo, &signalInfo);
    VK_CHECK(vkQueueSubmit2(ctx.graphicsQueue, 1, &submit, frame->renderFence));

    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.pSwapchains        = &ctx.swapchain.swapchain;
    presentInfo.swapchainCount     = 1;
    presentInfo.pWaitSemaphores    = &frame->renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo));

    ctx.frameIdx++;
}

static void render() {
    image_barrier(frame->commandBuffer, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background();

    image_barrier(frame->commandBuffer, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    image_barrier(frame->commandBuffer, ctx.framebuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    sceneData.proj = glm::perspective(glm::radians(70.f), (float)ctx.extent.width / (float)ctx.extent.height, 0.1f, 10000.f);
    sceneData.proj[1][1] *= -1;
    camera.update();
    sceneData.view = camera.view_matrix();
    draw_geometry();

    image_barrier(frame->commandBuffer, ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    image_barrier(frame->commandBuffer, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    blit(frame->commandBuffer, ctx.framebuffer.color[0].image, ctx.swapchain.images[swapchainImageIndex], ctx.extent, ctx.swapchain.extent);

    image_barrier(frame->commandBuffer, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    draw_imgui(ctx.swapchain.imageViews[swapchainImageIndex]);
    image_barrier(frame->commandBuffer, ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void spock::run() {
    using namespace std::chrono_literals;
    namespace stc = std::chrono;

    uint32_t fps = 0;
    uint32_t frameCounter = 0;
    stc::nanoseconds second(0);
    auto lastFrame = stc::steady_clock::now();

    while (!glfwWindowShouldClose(ctx.window)) {
        //#TODO: tick != fps
        tick = stc::nanoseconds(NS_PER_SEC / FPS_LIMIT);
        auto now = stc::steady_clock::now();
        if (now - lastFrame < tick && !FPS_UNLIMITED)
            continue;

        delta = now - lastFrame;
        second += delta;
        if (second >= stc::nanoseconds(NS_PER_SEC))
        {
            fps = frameCounter;
            frameCounter = 0;
            second -= stc::nanoseconds(NS_PER_SEC);
        }
        lastFrame = now;

        update_input();
        glfwPollEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("background")) {
            ImGui::SliderDouble("camera sensitivity", &camera.sensitivity, 0, 1.0);
            ImGui::SliderDouble("camera speed per ms", &camera.speed, 0, 0.1);
            ImGui::SliderInt("max fps", &FPS_LIMIT, 15, 240);
            ImGui::Checkbox("unlimited fps", &FPS_UNLIMITED);
            ImGui::Text("FPS: %d", fps);
            ImGui::Text("%d ms since last frame", int(delta.count() / NS_PER_MS));
        }
        ImGui::End();

        ImGui::Render();

        new_frame();
        render();
        end_frame();

        glfwMakeContextCurrent(ctx.window);
        glfwSwapBuffers(ctx.window);

        frameCounter++;
    }
}
