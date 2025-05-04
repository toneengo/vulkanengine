#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "info.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VkBootstrap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "lib/util.hpp"
#include "core.hpp"
#include "internal.hpp"
#include "destroy.hpp"
#include "shader.hpp"
#include "util.hpp"

#ifdef DBG
const bool gEnableValidationLayers = true;
#else
const bool gEnableValidationLayers = false;
#endif

using namespace spock;

static void destroy_swapchain() {
    vkDestroySwapchainKHR(ctx.device, ctx.swapchain.swapchain, nullptr);
    for (const auto& iv : ctx.swapchain.imageViews) {
        vkDestroyImageView(ctx.device, iv, nullptr);
    }
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    ctx.windowExtent.width  = width;
    ctx.windowExtent.height = height;
    destroy_swapchain();
    create_swapchain(width, height);
}

void error_exit() {
    cleanup();
    exit(1);
}

void spock::init_imgui() {
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
    VK_CHECK(vkCreateDescriptorPool(ctx.device, &pool_info, nullptr, &imguiPool));

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(ctx.window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion                = VK_API_VERSION_1_3; // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance                  = ctx.instance;
    init_info.PhysicalDevice            = ctx.physicalDevice;
    init_info.Device                    = ctx.device;
    init_info.QueueFamily               = ctx.graphicsQueueFamily;
    init_info.Queue                     = ctx.graphicsQueue;
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = imguiPool;
    init_info.UseDynamicRendering       = true;
    init_info.MinImageCount             = 3;
    init_info.ImageCount                = 3;
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    //dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &ctx.swapchain.imageFormat;
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    QUEUE_DESTROY_OBJ(imguiPool);
}

void spock::draw_imgui(VkImageView imageView) {
    const auto&               frame           = get_frame();
    const VkCommandBuffer     cmd             = frame.commandBuffer;
    VkRenderingAttachmentInfo colorAttachment = info::color_attachment(imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo           renderInfo      = info::rendering(ctx.swapchain.extent, &colorAttachment, nullptr);
    vkCmdBeginRendering(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

static void init_glfw_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    ctx.window              = glfwCreateWindow(800, 600, "vulkan engine", nullptr, nullptr);
    ctx.windowExtent.width  = 800;
    ctx.windowExtent.height = 600;
    ctx.monitor             = glfwGetPrimaryMonitor();
    auto mode               = glfwGetVideoMode(ctx.monitor);
    ctx.screenExtent.width  = mode->width;
    ctx.screenExtent.height = mode->height;
    ctx.screenExtent.depth  = 1;

    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);
}

static void init_device() {
    uint32_t             count;
    const char**         extensions = glfwGetRequiredInstanceExtensions(&count);

    vkb::InstanceBuilder builder;
    auto                 inst_ret = builder.set_app_name("vulkan app")
                        .request_validation_layers(gEnableValidationLayers)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .enable_extensions(count, extensions)
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    ctx.instance           = vkb_inst.instance;
    ctx.debugMessenger     = vkb_inst.debug_messenger;

    //glfw function pointers
    PFN_vkCreateInstance    pfnCreateInstance    = (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
    PFN_vkCreateDevice      pfnCreateDevice      = (PFN_vkCreateDevice)glfwGetInstanceProcAddress(ctx.instance, "vkCreateDevice");
    PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)glfwGetInstanceProcAddress(ctx.instance, "vkGetDeviceProcAddr");
    VK_CHECK(glfwCreateWindowSurface(ctx.instance, ctx.window, nullptr, &ctx.surface));

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features.dynamicRendering = true;
    features.synchronization2 = true;
    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress                           = true;
    features12.descriptorIndexing                            = true;
    features12.runtimeDescriptorArray                        = true;
    features12.shaderUniformBufferArrayNonUniformIndexing    = true;
    features12.shaderStorageImageArrayNonUniformIndexing     = true;
    features12.shaderStorageImageArrayNonUniformIndexing     = true;
    features12.descriptorBindingSampledImageUpdateAfterBind  = true;
    features12.descriptorBindingStorageBufferUpdateAfterBind = true;
    features12.descriptorBindingStorageImageUpdateAfterBind  = true;

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice         physical_device =
        selector.set_minimum_version(1, 3).set_required_features_13(features).set_required_features_12(features12).set_surface(ctx.surface).select().value();

    vkb::DeviceBuilder device_builder{physical_device};
    vkb::Device        vkb_device = device_builder.build().value();
    ctx.device                    = vkb_device.device;
    ctx.graphicsQueue             = vkb_device.get_queue(vkb::QueueType::graphics).value();
    ctx.graphicsQueueFamily       = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    ctx.physicalDevice            = physical_device.physical_device;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = ctx.physicalDevice;
    allocatorInfo.device                 = ctx.device;
    allocatorInfo.instance               = ctx.instance;
    allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &ctx.allocator);
    QUEUE_DESTROY_OBJ(ctx.allocator);
}

static void init_swapchain() {
    //initialize swapchain
    create_swapchain(ctx.windowExtent.width, ctx.windowExtent.height);
    //initialize main framebuffer
    ctx.framebuffer.color.resize(1);
    ctx.framebuffer.color[0] = create_image(ctx.screenExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    ctx.framebuffer.depth    = create_image(ctx.screenExtent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    for (const auto& image : ctx.framebuffer.color) {
        QUEUE_DESTROY_OBJ(image);
        QUEUE_DESTROY_OBJ(image.imageView);
    }
    QUEUE_DESTROY_OBJ(ctx.framebuffer.depth);
    QUEUE_DESTROY_OBJ(ctx.framebuffer.depth.imageView);
}

void init_commands() {
    //create frame command pools
    auto commandPoolInfo = info::create::command_pool(ctx.graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(ctx.device, &commandPoolInfo, nullptr, &ctx.frames[i].commandPool));
        VkCommandBufferAllocateInfo cmdAllocInfo = info::allocate::command_buffer(ctx.frames[i].commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(ctx.device, &cmdAllocInfo, &ctx.frames[i].commandBuffer));
    }

    VK_CHECK(vkCreateCommandPool(ctx.device, &commandPoolInfo, nullptr, &ctx.immCommandPool));
    VkCommandBufferAllocateInfo cmdAllocInfo = info::allocate::command_buffer(ctx.immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(ctx.device, &cmdAllocInfo, &ctx.immCommandBuffer));
    QUEUE_DESTROY_OBJ(ctx.immCommandPool);
}

static void init_synchronization() {
    //create synchronization structures
    VkFenceCreateInfo     fence     = info::create::fence(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore = info::create::semaphore();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(ctx.device, &fence, nullptr, &ctx.frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore, nullptr, &ctx.frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore, nullptr, &ctx.frames[i].renderSemaphore));
    }
    VK_CHECK(vkCreateFence(ctx.device, &fence, nullptr, &ctx.immCommandFence));
    QUEUE_DESTROY_OBJ(ctx.immCommandFence);
}

void spock::init() {
    init_glfw_window();
    init_device();
    init_swapchain();
    init_commands();
    init_synchronization();
    
    ctx.initialised = true;
}

void spock::clean_init() {
    clean_shader_modules();
}

VkDescriptorSetLayout spock::create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (auto& b : _bindings) {
        bindings.push_back({.binding = b.binding, .descriptorType = b.type, .descriptorCount = b.count, .stageFlags = shaderStages});
    }

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext                           = VK_NULL_HANDLE;
    info.pBindings                       = bindings.data();
    info.bindingCount                    = bindings.size();
    info.flags                           = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &set));
    QUEUE_DESTROY_OBJ(set);

    return set;
}

union DescriptorWriteInfo {
    VkDescriptorBufferInfo bufInfo;
    VkDescriptorImageInfo  imgInfo;
};
void spock::update_descriptor_sets(std::initializer_list<ImageWrite> imageWrites, std::initializer_list<BufferWrite> bufferWrites) {
    VkWriteDescriptorSet writeSets[32]; //max 32 writes for now
    DescriptorWriteInfo  writeInfos[32];

    int                  i = 0;
    for (auto& w : imageWrites) {
#ifdef DBG
        assert(i < 32);
#endif
        writeInfos[i].imgInfo.sampler     = w.sampler;
        writeInfos[i].imgInfo.imageView   = w.imageView;
        writeInfos[i].imgInfo.imageLayout = w.imageLayout;

        writeSets[i] = VkWriteDescriptorSet{
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = w.descriptorSet,
            .dstBinding       = w.binding,
            .dstArrayElement  = w.index,
            .descriptorCount  = w.count,
            .descriptorType   = w.descriptorType,
            .pImageInfo       = &writeInfos[i].imgInfo,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr //unused for now
        };

        i++;
    }
    for (auto& w : bufferWrites) {
#ifdef DBG
        assert(i < 32);
#endif
        writeInfos[i].bufInfo.buffer = w.buffer;
        writeInfos[i].bufInfo.range  = w.range;
        writeInfos[i].bufInfo.offset = w.offset;

        writeSets[i] = VkWriteDescriptorSet{
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = w.descriptorSet,
            .dstBinding       = w.binding,
            .dstArrayElement  = w.index,
            .descriptorCount  = w.count,
            .descriptorType   = w.descriptorType,
            .pImageInfo       = nullptr,
            .pBufferInfo      = &writeInfos[i].bufInfo,
            .pTexelBufferView = nullptr //unused for now
        };

        i++;
    }
    vkUpdateDescriptorSets(ctx.device, i, writeSets, 0, nullptr);
}

void spock::begin_immediate_command() {
    VK_CHECK(vkResetFences(ctx.device, 1, &ctx.immCommandFence));
    VK_CHECK(vkResetCommandBuffer(ctx.immCommandBuffer, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = info::begin::command_buffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(ctx.immCommandBuffer, &cmdBeginInfo));
}

void spock::end_immediate_command() {
    VK_CHECK(vkEndCommandBuffer(ctx.immCommandBuffer));
    VkCommandBufferSubmitInfo cmdinfo = info::submit::command_buffer(ctx.immCommandBuffer);
    VkSubmitInfo2             submit  = info::submit::submit(&cmdinfo, nullptr, nullptr);
    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(ctx.graphicsQueue, 1, &submit, ctx.immCommandFence));
    VK_CHECK(vkWaitForFences(ctx.device, 1, &ctx.immCommandFence, true, 9999999999));
}

Buffer spock::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    // allocate buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = nullptr;
    bufferInfo.size               = allocSize;
    bufferInfo.usage              = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memoryUsage;
    vmaallocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    Buffer buffer;
    VK_CHECK(vmaCreateBuffer(ctx.allocator, &bufferInfo, &vmaallocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
    return buffer;
}

Image spock::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
    size_t data_size    = size.depth * size.width * size.height * 4;
    Buffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    Image new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    begin_immediate_command();
    image_barrier(ctx.immCommandBuffer, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset      = 0;
    copyRegion.bufferRowLength   = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel       = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount     = 1;
    copyRegion.imageExtent                     = size;

    // copy the buffer into the image
    vkCmdCopyBufferToImage(ctx.immCommandBuffer, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    image_barrier(ctx.immCommandBuffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    end_immediate_command();

    vmaDestroyBuffer(ctx.allocator, uploadbuffer.buffer, uploadbuffer.allocation);

    return new_image;
}

Image spock::create_image(const char* fileName, VkImageUsageFlags usage, bool mipmapped)
{
    VkExtent3D     extent{};
    int            width = 0, height = 0;
    unsigned char* pixels = nullptr;
    bool empty = false;
    pixels = stbi_load(fileName, &width, &height, nullptr, 4);

    //create empty pixel image if couldn't load
    if (width == 0 || height == 0)
    {
        extent.width          = 1;
        extent.height         = 1;
        extent.depth          = 1;
        pixels = new unsigned char[4];
        memset(pixels, 0, sizeof(unsigned char)*4);
        empty = true;
    }
    else
    {
        extent.width          = width;
        extent.height         = height;
        extent.depth          = 1;
    }

    auto image = create_image(pixels, extent, VK_FORMAT_R8G8B8A8_UNORM, usage, mipmapped);
    if (empty) free(pixels);
    else stbi_image_free(pixels);
    return image;
}

Image spock::create_texture(const char* fileName, uint32_t index, VkDescriptorSet descriptorSet, uint32_t binding, VkSampler sampler, VkImageUsageFlags usage, bool mipmapped) {
    auto image = create_image(fileName, usage, mipmapped);
    update_descriptor_sets(
        {{descriptorSet, binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sampler, image.imageView, VK_IMAGE_LAYOUT_GENERAL, index}}, {});

    destroyQueue.push(image);
    destroyQueue.push(image.imageView);
    return image;
}

Image spock::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {
    Image newImage;
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo img_info = info::create::image(format, usage, size);
    if (mipmapped) {
        img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vmaCreateImage(ctx.allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build a image-view for the image
    VkImageViewCreateInfo view_info       = info::create::image_view(format, newImage.image, aspectFlag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(ctx.device, &view_info, nullptr, &newImage.imageView));

    return newImage;
}

void spock::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder{ctx.physicalDevice, ctx.device, ctx.surface};
    ctx.swapchain.imageFormat   = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      //.use_default_format_selection()
                                      .set_desired_format(VkSurfaceFormatKHR{.format = ctx.swapchain.imageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      //use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(width, height)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

    ctx.swapchain.extent = vkbSwapchain.extent;
    //store swapchain and its related images
    ctx.swapchain.swapchain  = vkbSwapchain.swapchain;
    ctx.swapchain.images     = vkbSwapchain.get_images().value();
    ctx.swapchain.imageViews = vkbSwapchain.get_image_views().value();
}

void spock::cleanup() {
    if (!ctx.initialised)
        return;

    vkDeviceWaitIdle(ctx.device);
    //ImGui_ImplVulkan_Shutdown();
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        vkDestroyCommandPool(ctx.device, ctx.frames[i].commandPool, nullptr);

        //destroy sync objects
        vkDestroyFence(ctx.device, ctx.frames[i].renderFence, nullptr);
        vkDestroySemaphore(ctx.device, ctx.frames[i].renderSemaphore, nullptr);
        vkDestroySemaphore(ctx.device, ctx.frames[i].swapchainSemaphore, nullptr);

        ctx.frames[i].descriptorAllocator.destroy_pools();
        ctx.frames[i].destroyQueue.flush();
    }

    ImGui_ImplVulkan_Shutdown();
    destroyQueue.flush();

    ctx.descriptorAllocator.destroy_pools();
    destroy_swapchain();

    vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    vkDestroyDevice(ctx.device, nullptr);

    vkb::destroy_debug_utils_messenger(ctx.instance, ctx.debugMessenger);
    vkDestroyInstance(ctx.instance, nullptr);
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
}

void spock::destroy_buffer(Buffer buffer) {
    vmaDestroyBuffer(ctx.allocator, buffer.buffer, buffer.allocation);
}

void* spock::get_mapped_data(VmaAllocation a) {
    return a->GetMappedData();
}
