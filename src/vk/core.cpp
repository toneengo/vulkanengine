#include "core.hpp"
#include "internal.hpp"
#include <fstream>
#include "lib/util.hpp"
#include "VkBootstrap.h"
#include "destroy.hpp"
#include "pipeline_builder.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <fstream>
#include <set>
#include "info.hpp"
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#ifdef DBG 
    const bool gEnableValidationLayers = true;
#else
    const bool gEnableValidationLayers = false;
#endif

using namespace engine;

static bool gResizeRequested = true;
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    gResizeRequested = true;
}

void error_exit()
{
    cleanup();
    exit(1);
}

VkImageSubresourceRange engine::image_subresource_range(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange subImage {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return subImage;
}

struct BarrierMask {
    VkPipelineStageFlags2      srcStageMask;
    VkAccessFlags2             srcAccessMask;
    VkPipelineStageFlags2      dstStageMask;
    VkAccessFlags2             dstAccessMask;
};

void engine::image_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask)
{
    VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.pNext = nullptr;

    imageBarrier.srcStageMask = srcStageMask;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstStageMask = dstStageMask;
    imageBarrier.dstAccessMask = dstAccessMask;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = image_subresource_range(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo depInfo {};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void engine::blit(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
    blitInfo.dstImage = dst;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

void engine::Object::destroy()
{
    switch (type)
    {
        case OBJ::Image:
            vmaDestroyImage(ctx.allocator, image, ctx.imageAllocations[image]);
            break;
        case OBJ::ImageView:
            vmaDestroyImage(ctx.allocator, image, ctx.imageAllocations[image]);
            break;
        case OBJ::Allocator:
            vmaDestroyAllocator(ctx.allocator);
            break;
        case OBJ::DescriptorPool:
            vkDestroyDescriptorPool(ctx.device, dp, nullptr);
            break;
        case OBJ::DescriptorSetLayout:
            vkDestroyDescriptorSetLayout(ctx.device, dsl, nullptr); 
            break;
        case OBJ::PipelineLayout:
            vkDestroyPipelineLayout(ctx.device, pll, nullptr);
            break;
        case OBJ::Pipeline:
            vkDestroyPipeline(ctx.device, pl, nullptr);
            break;
        case OBJ::Fence:
            vkDestroyFence(ctx.device, fence, nullptr);
            break;
        case OBJ::CommandPool:
            vkDestroyCommandPool(ctx.device, commandPool, nullptr);
            break;
        case OBJ::Buffer:
            vmaDestroyBuffer(ctx.allocator, buffer, ctx.bufferAllocations[buffer]);
            break;
        case OBJ::Sampler:
            vkDestroySampler(ctx.device, sampler, nullptr);
            break;
        default:
            break;
    }
}

void engine::DestroyQueue::push(Object obj)
{
    queue.push_back(obj);
}
void engine::DestroyQueue::flush()
{
    for (auto it = queue.rbegin(); it != queue.rend(); it++)
    {
        it->destroy();
    }
    queue.clear();
}

DestroyQueue destroyQueue;

//Shaders
//from glslang repo
class DirStackFileIncluder : public glslang::TShader::Includer {
public:
    DirStackFileIncluder() : externalLocalDirectoryCount(0) { }

    virtual IncludeResult* includeLocal(const char* headerName,
                                        const char* includerName,
                                        size_t inclusionDepth) override
    {
        return readLocalPath(headerName, includerName, (int)inclusionDepth);
    }

    virtual IncludeResult* includeSystem(const char* headerName,
                                         const char* /*includerName*/,
                                         size_t /*inclusionDepth*/) override
    {
        return readSystemPath(headerName);
    }

    // Externally set directories. E.g., from a command-line -I<dir>.
    //  - Most-recently pushed are checked first.
    //  - All these are checked after the parse-time stack of local directories
    //    is checked.
    //  - This only applies to the "local" form of #include.
    //  - Makes its own copy of the path.
    virtual void pushExternalLocalDirectory(const std::string& dir)
    {
        directoryStack.push_back(dir);
        externalLocalDirectoryCount = (int)directoryStack.size();
    }

    virtual void releaseInclude(IncludeResult* result) override
    {
        if (result != nullptr) {
            delete [] static_cast<tUserDataElement*>(result->userData);
            delete result;
        }
    }

    virtual std::set<std::string> getIncludedFiles()
    {
        return includedFiles;
    }

    virtual ~DirStackFileIncluder() override { }

protected:
    typedef char tUserDataElement;
    std::vector<std::string> directoryStack;
    int externalLocalDirectoryCount;
    std::set<std::string> includedFiles;

    // Search for a valid "local" path based on combining the stack of include
    // directories and the nominal name of the header.
    virtual IncludeResult* readLocalPath(const char* headerName, const char* includerName, int depth)
    {
        // Discard popped include directories, and
        // initialize when at parse-time first level.
        directoryStack.resize(depth + externalLocalDirectoryCount);
        if (depth == 1)
            directoryStack.back() = getDirectory(includerName);

        // Find a directory that works, using a reverse search of the include stack.
        for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); ++it) {
            std::string path = *it + '/' + headerName;
            std::replace(path.begin(), path.end(), '\\', '/');
            std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);
            if (file) {
                directoryStack.push_back(getDirectory(path));
                includedFiles.insert(path);
                return newIncludeResult(path, file, (int)file.tellg());
            }
        }

        return nullptr;
    }

    // Search for a valid <system> path.
    // Not implemented yet; returning nullptr signals failure to find.
    virtual IncludeResult* readSystemPath(const char* /*headerName*/) const
    {
        return nullptr;
    }

    // Do actual reading of the file, filling in a new include result.
    virtual IncludeResult* newIncludeResult(const std::string& path, std::ifstream& file, int length) const
    {
        char* content = new tUserDataElement [length];
        file.seekg(0, file.beg);
        file.read(content, length);
        return new IncludeResult(path, content, length, content);
    }

    // If no path markers, return current working directory.
    // Otherwise, strip file name and return path leading up to it.
    virtual std::string getDirectory(const std::string path) const
    {
        size_t last = path.find_last_of("/\\");
        return last == std::string::npos ? "." : path.substr(0, last);
    }
};

static std::vector<uint32_t> compileShaderToSPIRV_Vulkan(const char* const* shaderSource, EShLanguage stage)
{
    glslang::InitializeProcess();
    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory("assets/shaders");

    glslang::TShader shader(stage);
    shader.setDebugInfo(true);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    shader.setStrings(shaderSource, 1);
    if(!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault, includer))
    {
        puts(shader.getInfoLog());
        puts(shader.getInfoDebugLog());
        error_exit();
    }

    const char* const* ptr;
    int n = 0;
    shader.getStrings(ptr, n);

    glslang::TProgram program;
    program.addShader(&shader);
    program.link(EShMsgDefault);

    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    glslang::FinalizeProcess();

    return spirv;
}

VkDescriptorSetLayout engine::create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (auto& b : _bindings) {
        bindings.push_back({
            .binding = b.binding,
            .descriptorType = b.type,
            .descriptorCount = b.count,
            .stageFlags = shaderStages
        });
    }

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = VK_NULL_HANDLE;
    info.pBindings = bindings.data();
    info.bindingCount = bindings.size();
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &set));
    destroyQueue.push(set);

    return set;
}


VkShaderModule engine::create_shader_module(const char* filePath)
{
    // open the file. With cursor at the end
    const char* extension = filePath + strlen(filePath) - 4;
    EShLanguage stage;
    if (strcmp(extension, "vert") == 0)
        stage = EShLangVertex;
    if (strcmp(extension, "frag") == 0)
        stage = EShLangFragment;
    if (strcmp(extension, "comp") == 0)
        stage = EShLangCompute;

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        fmt::printf("Couldn't open file %s\n", filePath);
        error_exit();
        //return false;
    }

    int i = 0;
    char* buf = new char[2000000];
    while (file.get() > 0)
    {
        file.seekg(i);
        buf[i++] = file.get();
    }
    buf[i] = 0;
    file.close();

    auto spirv = compileShaderToSPIRV_Vulkan(&buf, stage);
    delete [] buf;

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(ctx.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        error_exit();
    }

    return shaderModule;
}

void engine::destroy_shader_module(VkShaderModule module)
{
    vkDestroyShaderModule(ctx.device, module, nullptr);
}

void init_glfw_window()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    ctx.window = glfwCreateWindow(800, 600, "vulkan engine", nullptr, nullptr);
    ctx.windowExtent.width  = 800;
    ctx.windowExtent.height = 600;
    ctx.monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(ctx.monitor);
    ctx.screenExtent.width  = mode->width; 
    ctx.screenExtent.height = mode->height; 
    ctx.screenExtent.depth  = 1; 

    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);
}

void init_core()
{
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("vulkan app")
        .request_validation_layers(gEnableValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .enable_extensions(count, extensions)
        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    ctx.instance = vkb_inst.instance;
    ctx.debugMessenger = vkb_inst.debug_messenger;

    //glfw function pointers
    PFN_vkCreateInstance pfnCreateInstance =
        (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
    PFN_vkCreateDevice pfnCreateDevice =
        (PFN_vkCreateDevice)glfwGetInstanceProcAddress(ctx.instance, "vkCreateDevice");
    PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr =
        (PFN_vkGetDeviceProcAddr)glfwGetInstanceProcAddress(ctx.instance, "vkGetDeviceProcAddr");
    VK_CHECK(glfwCreateWindowSurface(ctx.instance, ctx.window, nullptr, &ctx.surface));

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;
    //vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physical_device = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(ctx.surface)
		.select()
		.value();

	vkb::DeviceBuilder device_builder{ physical_device };
	vkb::Device vkb_device = device_builder.build().value();
    ctx.device = vkb_device.device;
	ctx.graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
	ctx.graphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    ctx.physicalDevice = physical_device.physical_device;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = ctx.physicalDevice;
    allocatorInfo.device = ctx.device;
    allocatorInfo.instance = ctx.instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &ctx.allocator);
    destroyQueue.push(ctx.allocator);

    //initialize swapchain
    create_swapchain(ctx.windowExtent.width, ctx.windowExtent.height);
    //initialize main framebuffer
    ctx.framebuffer.color.resize(1);
    ctx.framebuffer.color[0] = create_image(ctx.screenExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    ctx.framebuffer.depth = create_image(ctx.screenExtent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    for (const auto& image : ctx.framebuffer.color)
    {
        destroyQueue.push(image.image);
        destroyQueue.push(image.imageView);
    }
    destroyQueue.push(ctx.framebuffer.depth.image);
    destroyQueue.push(ctx.framebuffer.depth.imageView);

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
    destroyQueue.push(ctx.immCommandPool);

    //create synchronization structures
    VkFenceCreateInfo fence = info::create::fence(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphore = info::create::semaphore();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(ctx.device, &fence, nullptr, &ctx.frames[i].renderFence));
		VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore, nullptr, &ctx.frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(ctx.device, &semaphore, nullptr, &ctx.frames[i].renderSemaphore));
	}
    VK_CHECK(vkCreateFence(ctx.device, &fence, nullptr, &ctx.immCommandFence));
    destroyQueue.push(ctx.immCommandFence);

    //add
    //ctx.framebuffer.colorAttachments[0].descriptorSetLayout = create_descriptor_set_layout({ { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE } }, VK_SHADER_STAGE_COMPUTE_BIT);
    //ctx.framebuffer.depthAttachment.descriptorSetLayout = create_descriptor_set_layout({ { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } }, VK_SHADER_STAGE_FRAGMENT_BIT);

    //initialise descriptor allocators
    ctx.descriptorAllocator.init({
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }}, 10);
    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        ctx.frames[i].descriptorAllocator.init({
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 }}, 1000);
    }
}

void engine::init()
{
    init_glfw_window();
    init_core();
}

void engine::begin_immediate_command()
{
	VK_CHECK(vkResetFences(ctx.device, 1, &ctx.immCommandFence));
	VK_CHECK(vkResetCommandBuffer(ctx.immCommandBuffer, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = info::begin::command_buffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(ctx.immCommandBuffer, &cmdBeginInfo));
}

void engine::end_immediate_command()
{
	VK_CHECK(vkEndCommandBuffer(ctx.immCommandBuffer));

	VkCommandBufferSubmitInfo cmdinfo = info::submit::command_buffer(ctx.immCommandBuffer);
	VkSubmitInfo2 submit = info::submit::submit(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(ctx.graphicsQueue, 1, &submit, ctx.immCommandFence));

	VK_CHECK(vkWaitForFences(ctx.device, 1, &ctx.immCommandFence, true, 9999999999));

}

engine::ImmediateCommandGuard::ImmediateCommandGuard() {begin_immediate_command();}
engine::ImmediateCommandGuard::~ImmediateCommandGuard() {end_immediate_command();}

Buffer engine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	Buffer buffer;
	VK_CHECK(vmaCreateBuffer(ctx.allocator, &bufferInfo, &vmaallocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
	return buffer;
}

Image engine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	Buffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	Image new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    {
        auto _ = ImmediateCommandGuard();
		image_barrier(ctx.immCommandBuffer, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = size;

		// copy the buffer into the image
		vkCmdCopyBufferToImage(ctx.immCommandBuffer, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		image_barrier(ctx.immCommandBuffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    vmaDestroyBuffer(ctx.allocator, uploadbuffer.buffer, uploadbuffer.allocation);

	return new_image;
}

Image engine::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	Image newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = info::create::image(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK(vmaCreateImage(ctx.allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));
    ctx.imageAllocations[newImage.image] = newImage.allocation;

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = info::create::image_view(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(ctx.device, &view_info, nullptr, &newImage.imageView));

	return newImage;
}

VkPipeline create_default_graphics_pipeline(const char* vertPath, const char* fragPath)
{
    GraphicsPipelineBuilder builder;
    return builder
        .set_framebuffer(ctx.framebuffer)
        .set_shader_stages({
            {VK_SHADER_STAGE_VERTEX_BIT, create_shader_module(vertPath)},
            {VK_SHADER_STAGE_FRAGMENT_BIT, create_shader_module(fragPath)}
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
};

void engine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{ ctx.physicalDevice, ctx.device, ctx.surface };
	ctx.swapchain.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = ctx.swapchain.imageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	ctx.swapchain.extent = vkbSwapchain.extent;
	//store swapchain and its related images
	ctx.swapchain.swapchain = vkbSwapchain.swapchain;
	ctx.swapchain.images = vkbSwapchain.get_images().value();
	ctx.swapchain.imageViews = vkbSwapchain.get_image_views().value();
}

//VkPipeline create_default_compute_pipeline(const char* compPath)
//{
//};

void destroy_swapchain()
{
    vkDestroySwapchainKHR(ctx.device, ctx.swapchain.swapchain, nullptr);

	// destroy swapchain resources
	for (auto& siv : ctx.swapchain.imageViews) {
		vkDestroyImageView(ctx.device, siv, nullptr);
	}
}

void engine::cleanup()
{
    if (!ctx.initialised) return;

    vkDeviceWaitIdle(ctx.device);
    //ImGui_ImplVulkan_Shutdown();
    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        vkDestroyCommandPool(ctx.device, ctx.frames[i].commandPool, nullptr);

        //destroy sync objects
        vkDestroyFence(ctx.device, ctx.frames[i].renderFence, nullptr);
        vkDestroySemaphore(ctx.device, ctx.frames[i].renderSemaphore, nullptr);
        vkDestroySemaphore(ctx.device, ctx.frames[i].swapchainSemaphore, nullptr);

        ctx.frames[i].descriptorAllocator.destroy_pools();
        ctx.frames[i].destroyQueue.flush();
    }

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
