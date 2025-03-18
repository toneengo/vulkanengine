#include "vk/loader.hpp"
#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION

#include "util.hpp"
#include "create_info.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#include <cstring>
#include "lib/ds.hpp"
#include "lib/util.hpp"

#include "context.hpp"
#include "pipeline.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <glm/gtc/matrix_transform.hpp>

float translate_z = -5;

#ifdef DBG 
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

bool resize_requested = true;

void framebuffer_sz(GLFWwindow* window, int width, int height)
{
    resize_requested = true;
}

void VulkanContext::resize_swapchain()
{
    vkDeviceWaitIdle(device);

	destroy_swapchain();

	glfwGetWindowSize(window, &windowInfo.width, &windowInfo.height);

	create_swapchain(windowInfo.width, windowInfo.height);

	resize_requested = false;
}

void VulkanContext::init_glfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(800, 600, "vulkan engine", nullptr, nullptr);
    windowInfo.width = 800;
    windowInfo.height = 600;

    glfwSetFramebufferSizeCallback(window, framebuffer_sz);
}

void VulkanContext::init_triangle_pipeline()
{
    VkShaderModule triangleFragShader;
	if (!vkutil::load_shader_module("assets/shaders/colored_triangle.frag", device, &triangleFragShader)) {
		fmt::print("Error when building the triangle fragment shader module");
	}
	else {
		fmt::print("Triangle fragment shader succesfully loaded");
	}

	VkShaderModule triangleVertexShader;
	if (!vkutil::load_shader_module("assets/shaders/colored_triangle.vert", device, &triangleVertexShader)) {
		fmt::print("Error when building the triangle vertex shader module");
	}
	else {
		fmt::print("Triangle vertex shader succesfully loaded");
	}
	
	//build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &trianglePipelineLayout));
    PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder._pipelineLayout = trianglePipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	pipelineBuilder.disable_blending();
	//no depth testing
	pipelineBuilder.disable_depthtest();

	//connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(drawImage.imageFormat);
	pipelineBuilder.set_depth_format(VK_FORMAT_UNDEFINED);

	//finally build the pipeline
	trianglePipeline = pipelineBuilder.build_pipeline(device);

	//clean structures
	vkDestroyShaderModule(device, triangleFragShader, nullptr);
	vkDestroyShaderModule(device, triangleVertexShader, nullptr);

    mainDeletionQueue.push(trianglePipelineLayout);
    mainDeletionQueue.push(trianglePipeline);
}

void VulkanContext::init_mesh_pipeline()
{
    VkShaderModule triangleFragShader;
	if (!vkutil::load_shader_module("assets/shaders/tex_image.frag", device, &triangleFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
	}
	else {
		fmt::println("Triangle fragment shader succesfully loaded");
	}

	VkShaderModule triangleVertexShader;
	if (!vkutil::load_shader_module("assets/shaders/colored_triangle_mesh.vert", device, &triangleVertexShader)) {
		fmt::println("Error when building the triangle vertex shader module");
	}
	else {
		fmt::println("Triangle vertex shader succesfully loaded");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
	pipeline_layout_info.setLayoutCount = 1;
	VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &_meshPipelineLayout));
    PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder._pipelineLayout = _meshPipelineLayout;
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	//pipelineBuilder.disable_blending();
	pipelineBuilder.enable_blending_alphablend();

	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_LESS_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(drawImage.imageFormat);
	pipelineBuilder.set_depth_format(depthImage.imageFormat);

	//finally build the pipeline
	_meshPipeline = pipelineBuilder.build_pipeline(device);

	//clean structures
    mainDeletionQueue.push(_meshPipelineLayout);
    mainDeletionQueue.push(_meshPipeline);
	vkDestroyShaderModule(device, triangleFragShader, nullptr);
	vkDestroyShaderModule(device, triangleVertexShader, nullptr);

}
void VulkanContext::draw_geometry(VkCommandBuffer cmd)
{
	//begin a render pass  connected to our draw image
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo renderInfo = vkinit::rendering_info(drawExtent, &colorAttachment, &depthAttachment);

	vkCmdBeginRendering(cmd, &renderInfo);


	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = drawExtent.width;
	viewport.height = drawExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = drawExtent.width;
	scissor.extent.height = drawExtent.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

    /*
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);
    //launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);

    */
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

	GPUDrawPushConstants push_constants;
	push_constants.worldMatrix = glm::mat4{ 1.f };
	//push_constants.vertexBuffer = rectangle.vertexBufferAddress;
    //allocate a new uniform buffer for the scene data
	AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//add it to the deletion queue of this frame so it gets deleted once its been used
	get_current_frame().frameDeletionQueue.push(gpuSceneDataBuffer);

	//write the buffer
	GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
	*sceneUniformData = sceneData;

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = get_current_frame().frameDescriptors.allocate(device, _gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(device, globalDescriptor);

    //bind a texture
	VkDescriptorSet imageSet = get_current_frame().frameDescriptors.allocate(device, _singleImageDescriptorLayout);
	{
		DescriptorWriter writer;
		writer.write_image(0, _errorCheckerboardImage.imageView, _defaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		writer.update_set(device, imageSet);
	}

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &imageSet, 0, nullptr);

    /*
	vkCmdPushConstants(cmd, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
    */

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, translate_z));
	// camera projection
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)drawExtent.width / (float)drawExtent.height, 0.1f, 10000.0f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;
	push_constants.worldMatrix = projection * view;
    push_constants.vertexBuffer = testMeshes[2]->meshBuffers.vertexBufferAddress;

	vkCmdPushConstants(cmd, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, testMeshes[2]->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, testMeshes[2]->surfaces[0].count, 1, testMeshes[2]->surfaces[0].startIndex, 0, 0);

	vkCmdEndRendering(cmd);
}

void VulkanContext::init()
{
    init_glfw();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();
    init_default_data();
    init_imgui();

    initialised = true;
}

void VulkanContext::init_background_pipelines()
{
    VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ComputePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computeLayout.pPushConstantRanges = &pushConstant;
    computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &gradientPipelineLayout));

    //layout code
	VkShaderModule computeDrawShader;
	VkShaderModule gradientShader;
	if (!vkutil::load_shader_module("assets/shaders/gradient_color.comp", device, &gradientShader))
	{
		fmt::print("Error when building the compute shader \n");
	}
    VkShaderModule skyShader;
    if (!vkutil::load_shader_module("assets/shaders/sky.comp", device, &skyShader)) {
        fmt::print("Error when building the compute shader \n");
    }

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = gradientShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;
	
    ComputeEffect gradient;
    gradient.layout = gradientPipelineLayout;
    gradient.name = "gradient";
    gradient.data = {};

    //default colors
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &gradient.pipeline));
    ComputeEffect sky;
    sky.layout = gradientPipelineLayout;
    sky.name = "sky";
    sky.data = {};
    //default sky parameters
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);

    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

    backgroundEffects.push_back(gradient);
    backgroundEffects.push_back(sky);

    vkDestroyShaderModule(device, gradientShader, nullptr);
    vkDestroyShaderModule(device, skyShader, nullptr);

    mainDeletionQueue.push(gradientPipelineLayout);
    mainDeletionQueue.push(sky.pipeline);
    mainDeletionQueue.push(gradient.pipeline);
}
void VulkanContext::init_default_data() {
	std::array<Vertex,4> rect_vertices;

	rect_vertices[0].position = {0.5,-0.5, 0};
	rect_vertices[1].position = {0.5,0.5, 0};
	rect_vertices[2].position = {-0.5,-0.5, 0};
	rect_vertices[3].position = {-0.5,0.5, 0};

	rect_vertices[0].color = {0,0, 0,1};
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32_t,6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	rectangle = upload_mesh(rect_indices,rect_vertices);

    testMeshes = loadGltfMeshes(this, "assets/meshes/basicmesh.glb").value();

    //3 default textures, white, grey, black. 1 pixel each
	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	_whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	_greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	_blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	//checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	_errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(device, &sampl, nullptr, &_defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(device, &sampl, nullptr, &_defaultSamplerLinear);

	mainDeletionQueue.push(_defaultSamplerNearest);
	mainDeletionQueue.push(_defaultSamplerLinear);
    mainDeletionQueue.push(_whiteImage);
    mainDeletionQueue.push(_greyImage);
    mainDeletionQueue.push(_blackImage);
    mainDeletionQueue.push(_errorCheckerboardImage);
	//delete the rectangle data on engine shutdown
    mainDeletionQueue.push(rectangle.indexBuffer);
    mainDeletionQueue.push(rectangle.vertexBuffer);
}

void VulkanContext::init_pipelines()
{
    init_background_pipelines();
    init_triangle_pipeline();
    init_mesh_pipeline();
}

void VulkanContext::init_descriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	globalDescriptorAllocator.init(device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		drawImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
	}
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _singleImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout = builder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    // other code
	//allocate a descriptor set for our draw image
	drawImageDescriptors = globalDescriptorAllocator.allocate(device,drawImageDescriptorLayout);	

    DescriptorWriter writer;
    writer.write_image(0, drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    writer.update_set(device,drawImageDescriptors);
    for (int i = 0; i < FRAME_OVERLAP; i++) {
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = { 
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		frames[i].frameDescriptors = DescriptorAllocatorGrowable{};
		frames[i].frameDescriptors.init(device, 1000, frame_sizes);
	
        mainDeletionQueue.push(&frames[i].frameDescriptors);
	}

    mainDeletionQueue.push(&globalDescriptorAllocator);
    mainDeletionQueue.push(drawImageDescriptorLayout);
}

void VulkanContext::init_vulkan()
{
    uint32_t count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    vkb::InstanceBuilder builder;
	//make the vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(enableValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
        .enable_extensions(count, extensions)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance 
	instance = vkb_inst.instance;

    //glfw function pointers
    PFN_vkCreateInstance pfnCreateInstance =
        (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
    PFN_vkCreateDevice pfnCreateDevice =
        (PFN_vkCreateDevice)glfwGetInstanceProcAddress(instance, "vkCreateDevice");
    PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr =
        (PFN_vkGetDeviceProcAddr)glfwGetInstanceProcAddress(instance, "vkGetDeviceProcAddr");

	debugMessenger = vkb_inst.debug_messenger;

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;
    //vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(surface)
		.select()
		.value();

    //create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();
    // Get the VkDevice handle used in the rest of a vulkan application
	device = vkbDevice.device;
	chosenGPU = physicalDevice.physical_device;

    // use vkbootstrap to get a Graphics queue
	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenGPU;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    mainDeletionQueue.queue.push_back(allocator);
}

void VulkanContext::init_commands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
    auto commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	
	for (int i = 0; i < FRAME_OVERLAP; i++) {
        frames[i].frameDeletionQueue.ctx = this;
		VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));
	}

    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &immCommandPool));
	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(immCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &immCommandBuffer));
    mainDeletionQueue.push(immCommandPool);
}

void VulkanContext::init_imgui()
{
    // this initializes the core structures of imgui
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 0;
    for (VkDescriptorPoolSize& pool_size : pool_sizes)
        pool_info.maxSets += pool_size.descriptorCount;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));

	ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = instance;
    init_info.PhysicalDevice = chosenGPU;
    init_info.Device = device;
    init_info.QueueFamily = graphicsQueueFamily;
    init_info.Queue = graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imguiPool;
    init_info.UseDynamicRendering = true;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    //dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();


    mainDeletionQueue.push(imguiPool);
}

void VulkanContext::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::rendering_info(swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void VulkanContext::init_sync_structures()
{
    //create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
	}
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immFence));
    mainDeletionQueue.push(immFence);
}

void VulkanContext::immediate_submit(std::function<void(VkCommandBuffer)> function)
{
	VK_CHECK(vkResetFences(device, 1, &immFence));
	VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));

	VkCommandBuffer cmd = immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immFence));

	VK_CHECK(vkWaitForFences(device, 1, &immFence, true, 9999999999));
}

void VulkanContext::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{ chosenGPU,device,surface };

	swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	swapchainExtent = vkbSwapchain.extent;
	//store swapchain and its related images
	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanContext::init_swapchain()
{
    create_swapchain(windowInfo.width, windowInfo.height);
    //draw image size will match the window
	VkExtent3D drawImageExtent = {
		1920,
		1080,
		1
	};

	//hardcoding the draw format to 32 bit float
	drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::image_create_info(drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(allocator, &rimg_info, &rimg_allocinfo, &drawImage.image, &drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &drawImage.imageView));

    depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::image_create_info(depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(allocator, &dimg_info, &rimg_allocinfo, &depthImage.image, &depthImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(depthImage.imageFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &dview_info, nullptr, &depthImage.imageView));

	//add to deletion queues
	mainDeletionQueue.push(drawImage);
	mainDeletionQueue.push(depthImage);
}

void VulkanContext::destroy_swapchain()
{
	vkDestroySwapchainKHR(device, swapchain, nullptr);

	// destroy swapchain resources
	for (auto& siv : swapchainImageViews) {
		vkDestroyImageView(device, siv, nullptr);
	}
}

void VulkanContext::render_loop()
{
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (resize_requested) resize_swapchain();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		
		if (ImGui::Begin("background")) {
			
			ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
		
			ImGui::Text("Selected effect: %s", selected.name);
		
			ImGui::SliderInt("Effect Index", &currentBackgroundEffect,0, backgroundEffects.size() - 1);
		
			ImGui::InputFloat4("data1",(float*)& selected.data.data1);
			ImGui::InputFloat4("data2",(float*)& selected.data.data2);
			ImGui::InputFloat4("data3",(float*)& selected.data.data3);
			ImGui::InputFloat4("data4",(float*)& selected.data.data4);
			ImGui::SliderFloat("trans", &translate_z, -10.0, 10.0);
		}
		ImGui::End();

        ImGui::Render();
        draw();
    }
}

void VulkanContext::draw_background(VkCommandBuffer cmd)
{
    /*
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(frameNumber / 120.f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clearRange = vkutil::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    //clear image
	vkCmdClearColorImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    */
    // bind the gradient drawing compute pipeline
    ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout, 0, 1, &drawImageDescriptors, 0, nullptr);

	vkCmdPushConstants(cmd, gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(drawExtent.width / 16.0), std::ceil(drawExtent.height / 16.0), 1);

}
AllocatedBuffer VulkanContext::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;

	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	AllocatedBuffer newBuffer;

	// allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
		&newBuffer.info));

	return newBuffer;
}

void VulkanContext::destroy_buffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}
GPUMeshBuffers VulkanContext::upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers newSurface;

	//create vertex buffer
	newSurface.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

	//create index buffer
	newSurface.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);
    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

	immediate_submit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	destroy_buffer(staging);

	return newSurface;
}
void VulkanContext::draw()
{
	// wait until the gpu has finished rendering the last frame. Timeout of 1
	// second
	VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().renderFence, true, 1000000000));
    get_current_frame().frameDeletionQueue.flush();
	get_current_frame().frameDescriptors.clear_pools(device);
	VK_CHECK(vkResetFences(device, 1, &get_current_frame().renderFence));
    
    //request image from the swapchain
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, get_current_frame().swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = get_current_frame().mainCommandBuffer;

    // we are sure commands finished executing
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    drawExtent.width = std::min(drawImage.imageExtent.width, swapchainExtent.width) * renderScale;
    drawExtent.height = std::min(drawImage.imageExtent.height, swapchainExtent.height) * renderScale;

    vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);
    vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    vkutil::transition_image(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	draw_geometry(cmd);


    vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkutil::blit_image(cmd, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	draw_imgui(cmd, swapchainImageViews[swapchainImageIndex]);

	//make the swapchain image into presentable mode
    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));
    //prepare the submission to the queue. 
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);	
	
	VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,get_current_frame().swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame().renderSemaphore);	
	
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo,&signalInfo,&waitInfo);	

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, get_current_frame().renderFence));

    //prepare present
	// this will put the image we just rendered to into the visible window.
	// we want to wait on the _renderSemaphore for that, 
	// as its necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

	//increase the number of frames drawn
	frameNumber++;
}

void VulkanContext::cleanup()
{
	if (initialised) {
        vkDeviceWaitIdle(device);
        ImGui_ImplVulkan_Shutdown();
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

            //destroy sync objects
            vkDestroyFence(device, frames[i].renderFence, nullptr);
            vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(device ,frames[i].swapchainSemaphore, nullptr);

            frames[i].frameDeletionQueue.flush();
        }
        for (auto& mesh : testMeshes) {
            destroy_buffer(mesh->meshBuffers.indexBuffer);
            destroy_buffer(mesh->meshBuffers.vertexBuffer);
        }

        mainDeletionQueue.flush();

		destroy_swapchain();

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
		
		vkb::destroy_debug_utils_messenger(instance, debugMessenger);
		vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();

	}
}

AllocatedImage VulkanContext::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage VulkanContext::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);

	AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediate_submit([&](VkCommandBuffer cmd) {
		vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

	destroy_buffer(uploadbuffer);

	return new_image;
}

void VulkanContext::destroy_image(const AllocatedImage& img)
{
    vkDestroyImageView(device, img.imageView, nullptr);
    vmaDestroyImage(allocator, img.image, img.allocation);
}
