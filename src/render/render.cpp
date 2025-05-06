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
#include "texgui.h"
#include "spock/core.hpp"
#include "spock/info.hpp"
#include "spock/internal.hpp"
#include "spock/pipeline_builder.hpp"
#include "spock/util.hpp"
#include "input.hpp"
#include "mesh.hpp"
#include "render.hpp"

#include "render_data.hpp"

using namespace vkengine;
uint32_t currentTextureIndex = 0;

uint32_t selected = 0;
TexGui::RenderData data; 
TexGui::RenderData copy; 
char charbuf[128] = "\0";

// extending imgui with some helper functions
namespace ImGui {
    inline bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
    {
        return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
    }
}

static void init_imgui() {
    // this initializes the core structures of imgui
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                    = 0;

    for (VkDescriptorPoolSize& pool_size : pool_sizes)
        pool_info.maxSets += pool_size.descriptorCount;

    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes    = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(spock::ctx.device, &pool_info, nullptr, &imguiPool));

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(spock::ctx.window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion                = VK_API_VERSION_1_3; // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance                  = spock::ctx.instance;
    init_info.PhysicalDevice            = spock::ctx.physicalDevice;
    init_info.Device                    = spock::ctx.device;
    init_info.QueueFamily               = spock::ctx.graphicsQueueFamily;
    init_info.Queue                     = spock::ctx.graphicsQueue;
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = imguiPool;
    init_info.UseDynamicRendering       = true;
    init_info.MinImageCount             = 3;
    init_info.ImageCount                = 3;
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    //dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &spock::ctx.swapchain.imageFormat;
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    QUEUE_DESTROY_OBJ(imguiPool);
}

static void init_texgui() {
    // this initializes the core structures of imgui

    TexGui::Defaults::PixelSize = 2;
    TexGui::Defaults::Font::Size = 20;
    TexGui::Defaults::Font::MsdfPxRange = 2;

    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    pool_info.maxSets                    = 0;

    for (VkDescriptorPoolSize& pool_size : pool_sizes)
        pool_info.maxSets += pool_size.descriptorCount;

    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    pool_info.pPoolSizes    = pool_sizes;

    VkDescriptorPool texguiPool;
    VK_CHECK(vkCreateDescriptorPool(spock::ctx.device, &pool_info, nullptr, &texguiPool));

    TexGui::VulkanInitInfo init_info = {};
    init_info.ApiVersion                = VK_API_VERSION_1_3; // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance                  = spock::ctx.instance;
    init_info.PhysicalDevice            = spock::ctx.physicalDevice;
    init_info.Device                    = spock::ctx.device;
    init_info.QueueFamily               = spock::ctx.graphicsQueueFamily;
    init_info.Queue                     = spock::ctx.graphicsQueue;
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = texguiPool;
    init_info.UseDynamicRendering       = true;
    init_info.MinImageCount             = 3;
    init_info.ImageCount                = 3;
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    //dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &spock::ctx.swapchain.imageFormat;
    init_info.Allocator = spock::ctx.allocator;

    TexGui::initGlfwVulkan(spock::ctx.window, init_info);
    TexGui::loadFont("assets/fonts/Jersey10-Regular.ttf");
    TexGui::loadTextures("assets/sprites");

    QUEUE_DESTROY_OBJ(texguiPool);

}

static void draw_imgui(VkImageView imageView) {
    const auto&               frame           = spock::get_frame();
    const VkCommandBuffer     cmd             = frame.commandBuffer;
    VkRenderingAttachmentInfo colorAttachment = info::color_attachment(imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo           renderInfo      = info::rendering(spock::ctx.swapchain.extent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

static void draw_texgui(VkImageView imageView, const TexGui::RenderData& rs) {
    const auto&               frame           = spock::get_frame();
    const VkCommandBuffer     cmd             = frame.commandBuffer;
    VkRenderingAttachmentInfo colorAttachment = info::color_attachment(imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo           renderInfo      = info::rendering(spock::ctx.swapchain.extent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);
    TexGui::render_Vulkan(rs, cmd);
    vkCmdEndRendering(cmd);
}

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

void vkengine::init_engine() {
    spock::init();

    //initialise descriptor allocators
    spock::ctx.descriptorAllocator.set_flags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    spock::ctx.descriptorAllocator.init(
        {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMAGE_COUNT}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SAMPLER_COUNT}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, STORAGE_COUNT}}, 1);
    for (int i = 0; i < spock::FRAME_OVERLAP; i++) {
        spock::ctx.frames[i].descriptorAllocator.init(
            {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}},
            1000);
    }

    //create default samplers
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampl.magFilter           = VK_FILTER_NEAREST;
    sampl.minFilter           = VK_FILTER_NEAREST;
    vkCreateSampler(spock::ctx.device, &sampl, nullptr, &nearestSampler);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(spock::ctx.device, &sampl, nullptr, &linearSampler);

    QUEUE_DESTROY_OBJ(linearSampler);
    QUEUE_DESTROY_OBJ(nearestSampler);

    //create descriptor set layouts
    samplerDescriptorSetLayout = spock::create_descriptor_set_layout(
        {{SAMPLER_BINDING, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SAMPLER_COUNT}},
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

    samplerDescriptorSet = spock::ctx.descriptorAllocator.allocate(samplerDescriptorSetLayout);

    init_input_callbacks();
    init_imgui();
    init_texgui();

    VkShaderModule gradient = spock::create_shader_module("assets/shaders/gradient.comp");
    computeImageDescLayout  = spock::create_descriptor_set_layout({{0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}}, VK_SHADER_STAGE_COMPUTE_BIT);
    uniformDescLayout       = spock::create_descriptor_set_layout({{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}}, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    computeImageDesc        = spock::ctx.descriptorAllocator.allocate(computeImageDescLayout);

    spock::update_descriptor_sets({{computeImageDesc, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, spock::ctx.framebuffer.color[0].imageView, VK_IMAGE_LAYOUT_GENERAL}}, {});

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
                             .set_framebuffer(spock::ctx.framebuffer)
                             .set_shader_stages({{VK_SHADER_STAGE_VERTEX_BIT, spock::create_shader_module("assets/shaders/mesh.vert")},
                                                 {VK_SHADER_STAGE_FRAGMENT_BIT, spock::create_shader_module("assets/shaders/mesh.frag")}})
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
    for (auto& mesh: guitar.meshes)
    {
        create_texture(mesh.diffuse, currentTextureIndex++, samplerDescriptorSet, SAMPLER_BINDING, linearSampler);
        create_texture(mesh.normal, currentTextureIndex++, samplerDescriptorSet, SAMPLER_BINDING, linearSampler);
        create_texture(mesh.specular, currentTextureIndex++, samplerDescriptorSet, SAMPLER_BINDING, linearSampler);
    }

    spock::clean_init();
}

uint32_t swapchainImageIndex = 0;
static spock::FrameContext* frame = nullptr;
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
    vkCmdDispatch(frame->commandBuffer, std::ceil(spock::ctx.extent.width / 16.0), std::ceil(spock::ctx.extent.height / 16.0), 1);
}

void draw_geometry() {

    VkRenderingAttachmentInfo colorAttachment = info::color_attachment(spock::ctx.framebuffer.color[0].imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = info::depth_attachment(spock::ctx.framebuffer.depth.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo           renderInfo = info::rendering(spock::ctx.extent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(frame->commandBuffer, &renderInfo);

    vkCmdBindPipeline(frame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vertexPipeline);

    // set dynamic viewport and scissor
    spock::set_viewport(frame->commandBuffer, 0, 0, spock::ctx.extent.width, spock::ctx.extent.height);
    spock::set_scissor(frame->commandBuffer, 0, 0, spock::ctx.extent.width, spock::ctx.extent.height);

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
    frame = &spock::get_frame();
    VK_CHECK(vkWaitForFences(spock::ctx.device, 1, &frame->renderFence, true, 1000000000));
    frame->destroyQueue.flush();
    frame->descriptorAllocator.clear_pools();
    VK_CHECK(vkResetFences(spock::ctx.device, 1, &frame->renderFence));

    VK_CHECK(vkAcquireNextImageKHR(spock::ctx.device, spock::ctx.swapchain.swapchain, 1000000000, frame->swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = frame->commandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = info::begin::command_buffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    spock::ctx.extent.width  = std::min(spock::ctx.screenExtent.width, spock::ctx.swapchain.extent.width) * spock::ctx.renderScale;
    spock::ctx.extent.height = std::min(spock::ctx.screenExtent.height, spock::ctx.swapchain.extent.height) * spock::ctx.renderScale;

    VK_CHECK(vkBeginCommandBuffer(frame->commandBuffer, &cmdBeginInfo));
}

static void end_frame() {
    VK_CHECK(vkEndCommandBuffer(frame->commandBuffer));

    VkCommandBufferSubmitInfo cmdInfo    = info::submit::command_buffer(frame->commandBuffer);
    VkSemaphoreSubmitInfo     waitInfo   = info::submit::semaphore(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame->swapchainSemaphore);
    VkSemaphoreSubmitInfo     signalInfo = info::submit::semaphore(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame->renderSemaphore);
    VkSubmitInfo2             submit     = info::submit::submit(&cmdInfo, &waitInfo, &signalInfo);
    VK_CHECK(vkQueueSubmit2(spock::ctx.graphicsQueue, 1, &submit, frame->renderFence));

    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.pSwapchains        = &spock::ctx.swapchain.swapchain;
    presentInfo.swapchainCount     = 1;
    presentInfo.pWaitSemaphores    = &frame->renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(spock::ctx.graphicsQueue, &presentInfo));

    spock::ctx.frameIdx++;
}

static void render() {
    spock::image_barrier(frame->commandBuffer, spock::ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background();

    spock::image_barrier(frame->commandBuffer, spock::ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    spock::image_barrier(frame->commandBuffer, spock::ctx.framebuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    sceneData.proj = glm::perspective(glm::radians(70.f), (float)spock::ctx.extent.width / (float)spock::ctx.extent.height, 0.1f, 10000.f);
    sceneData.proj[1][1] *= -1;
    camera.update();
    sceneData.view = camera.view_matrix();
    draw_geometry();

    spock::image_barrier(frame->commandBuffer, spock::ctx.framebuffer.color[0].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    spock::image_barrier(frame->commandBuffer, spock::ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    spock::blit(frame->commandBuffer, spock::ctx.framebuffer.color[0].image, spock::ctx.swapchain.images[swapchainImageIndex], spock::ctx.extent, spock::ctx.swapchain.extent);

    spock::image_barrier(frame->commandBuffer, spock::ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    draw_imgui(spock::ctx.swapchain.imageViews[swapchainImageIndex]);
    draw_texgui(spock::ctx.swapchain.imageViews[swapchainImageIndex], copy);
    spock::image_barrier(frame->commandBuffer, spock::ctx.swapchain.images[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

static inline void add_texgui_widgets()
{
    data.Base.Window("not bob", 400, 500, 200, 200, TexGui::CENTER_X | TexGui::CENTER_Y);
    auto win = data.Base.Window("bob", 200, 100, 400, 600, TexGui::RESIZABLE);

    auto row = win.Row({200, 0});

    static bool a = false;
    static bool b = false;
    static bool c = true;

    static uint32_t selected2 = 0;
    auto box0 = row[0].Column({24, 24, 24, 24, 24});
    auto checkBoxCon = box0[0].Row({24, 0});
    checkBoxCon[0].CheckBox(&a);
    checkBoxCon[1].Align(TexGui::ALIGN_CENTER_Y).Text("value a");

    checkBoxCon = box0[1].Row({24, 0});
    checkBoxCon[0].RadioButton(&selected2, 0);
    checkBoxCon[1].Align(TexGui::ALIGN_CENTER_Y).Text("value b");

    checkBoxCon = box0[2].Row({24, 0});
    checkBoxCon[0].RadioButton(&selected2, 1);
    checkBoxCon[1].Align(TexGui::ALIGN_CENTER_Y).Text("value c");

    static std::string input;
    box0[3].TextInput("std::string input...", input);
    box0[4].TextInput("char input...", charbuf, 128);

    auto box1 = row[1].ScrollPanel("panel1", TexGui::texByName("box2"));
    auto grid = box1.Grid();

    for (uint32_t i = 0; i < 10; i++)
    {
        grid
            .ListItem(&selected, i)
            .Image(TexGui::texByName("lollipop"));
    }

    copy.copy(data);
}
void vkengine::run() {
    using namespace std::chrono_literals;
    namespace stc = std::chrono;

    uint32_t fps = 0;
    uint32_t frameCounter = 0;
    stc::nanoseconds second(0);
    auto lastFrame = stc::steady_clock::now();

    while (!glfwWindowShouldClose(spock::ctx.window)) {
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
        add_texgui_widgets();

        TexGui::newFrame();

        new_frame();
        render();
        end_frame();

        data.clear();
        TexGui::clear();

        glfwMakeContextCurrent(spock::ctx.window);
        glfwSwapBuffers(spock::ctx.window);

        frameCounter++;
    }
}

void vkengine::cleanup()
{
    spock::cleanup();
}
