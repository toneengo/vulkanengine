#include "pipeline_builder.hpp"
#include "internal.hpp"
#include "lib/util.hpp"
#include <vulkan/vulkan_core.h>

VkPipelineColorBlendAttachmentState color_blend(Blend color, Blend alpha)
{
    return {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = color.src,
        .dstColorBlendFactor = color.dst,
        .colorBlendOp = color.op,
        .srcAlphaBlendFactor = alpha.src,
        .dstAlphaBlendFactor = alpha.dst,
        .alphaBlendOp = alpha.op,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
}

VkPipelineColorBlendAttachmentState color_blend()
{
    VkPipelineColorBlendAttachmentState state = {};
    state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    state.blendEnable = VK_FALSE;
    return state;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_descriptor_set_layouts(std::initializer_list<VkDescriptorSetLayout> dsLayouts)
{
    descriptorSetLayouts = dsLayouts;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_push_constant_ranges(std::initializer_list<VkPushConstantRange> ranges)
{
    pushConstantRanges = ranges;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_framebuffer(engine::Framebuffer fb)
{
    framebuffer = fb;
    return *this;
}


GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_shader_stages(std::initializer_list<ShaderStage> shaderStages)
{
    for (const auto& stage : shaderStages)
    {
        VkPipelineShaderStageCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;
        info.stage = stage.stage;
        info.module = stage.module;
        info.pName = "main";
        info.pSpecializationInfo = VK_NULL_HANDLE;
        stages.push_back(info);
    }
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_viewport_state(uint32_t viewportCount, uint32_t scissorCount, VkViewport* viewports, VkRect2D* scissors)
{
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.viewportCount = viewportCount;
    viewportState.scissorCount = scissorCount;
    viewportState.pViewports = viewports;
    viewportState.pScissors = scissors;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_color_blend_states(std::initializer_list<VkPipelineColorBlendAttachmentState> attachments)
{
    colorBlendAttachmentStates = attachments;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_input_assembly_state(VkPrimitiveTopology topology, bool primitiveRestartEnable)
{
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = topology;
    inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_rasterization_state(VkPolygonMode polygonMode, float lineWidth, VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = polygonMode;
    rasterizationState.lineWidth = lineWidth;
    rasterizationState.cullMode = cullMode;
    rasterizationState.frontFace = frontFace;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_multisample_state()
{
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;
    return *this;
}
//sample shading disabled by default
GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_multisample_state(int rasterizationSamples, bool alphaToCoverageEnable, bool alphaToOneEnable)
{
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.rasterizationSamples = VkSampleCountFlagBits(rasterizationSamples);
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = alphaToCoverageEnable;
    multisampleState.alphaToOneEnable = alphaToOneEnable;
    return *this;
}

static VkPipelineDepthStencilStateCreateInfo _pipeline_depth_stencil_state_create_info(
    bool depthTestEnable, bool depthWriteEnable, VkCompareOp compareOp, bool depthBoundsTestEnable,
    bool stencilTestEnable, VkStencilOpState front, VkStencilOpState back,
    float minDepthBounds, float maxDepthBounds)
{
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTestEnable;
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = compareOp;
    depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
    depthStencil.stencilTestEnable = stencilTestEnable;
    depthStencil.front = front;
    depthStencil.back = back;
    depthStencil.minDepthBounds = minDepthBounds;
    depthStencil.maxDepthBounds = maxDepthBounds;
    return depthStencil;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_depth_stencil_state(VkCompareOp compareOp, bool writeable, VkStencilOpState front, VkStencilOpState back, float minDepthBounds, float maxDepthBounds)
{
    depthStencilState = _pipeline_depth_stencil_state_create_info(true, writeable, compareOp, true, true, front, back, minDepthBounds, maxDepthBounds);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_depth_stencil_state(VkCompareOp compareOp, bool writeable, VkStencilOpState front, VkStencilOpState back)
{
    depthStencilState = _pipeline_depth_stencil_state_create_info(true, writeable, compareOp, false, true, front, back, 0.0, 1.0);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_stencil_state(VkStencilOpState front, VkStencilOpState back)
{
    depthStencilState = _pipeline_depth_stencil_state_create_info(false, false, VK_COMPARE_OP_NEVER, false, true, front, back, 0.0, 1.0);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_depth_state(VkCompareOp compareOp, bool writeable)
{
    depthStencilState = _pipeline_depth_stencil_state_create_info(true, writeable, compareOp, false, false, {}, {}, 0.0, 1.0);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_depth_state(VkCompareOp compareOp, bool writeable, float minDepthBounds, float maxDepthBounds)
{
    depthStencilState = _pipeline_depth_stencil_state_create_info(true, writeable, compareOp, true, false, {}, {}, minDepthBounds, maxDepthBounds);
    return *this;
}
GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_dynamic_states(std::initializer_list<VkDynamicState> states)
{
    dynamicStates = states;
    return *this;
}

VkPipeline GraphicsPipelineBuilder::build()
{
    VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.pSetLayouts = descriptorSetLayouts.data();
	layoutInfo.setLayoutCount = descriptorSetLayouts.size();
    layoutInfo.pPushConstantRanges = pushConstantRanges.data();
    layoutInfo.pushConstantRangeCount = pushConstantRanges.size();
	VK_CHECK(vkCreatePipelineLayout(engine::ctx.device, &layoutInfo, nullptr, &layout));

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    std::vector<VkFormat> colorAttachmentFormats;
    for (auto& c : framebuffer.color)
    {
        colorAttachmentFormats.push_back(c.imageFormat);
    }
    VkPipelineRenderingCreateInfo _r = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .viewMask = framebuffer.viewMask,
        .colorAttachmentCount = uint32_t(colorAttachmentFormats.size()),
        .pColorAttachmentFormats = colorAttachmentFormats.data(),
        .depthAttachmentFormat = framebuffer.depth.imageFormat,
        .stencilAttachmentFormat = framebuffer.stencil.imageFormat,
    };
    info.pNext = &_r;
    info.flags = flags;
    info.stageCount = stages.size();
    info.pStages = stages.data();
    info.pVertexInputState = &vertexInputState;
    info.pInputAssemblyState = &inputAssemblyState;
    info.pTessellationState = &tessellationState;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &rasterizationState;
    info.pMultisampleState = &multisampleState;
    info.pDepthStencilState = &depthStencilState;
    VkPipelineColorBlendStateCreateInfo _c = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = uint32_t(colorBlendAttachmentStates.size()),
        .pAttachments = colorBlendAttachmentStates.data(),
    };

    info.pColorBlendState = &_c;
    VkPipelineDynamicStateCreateInfo _d = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .dynamicStateCount = uint32_t(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    info.pDynamicState = &_d;
    info.layout = layout;
    //the rest are unused parameters
    
    if (vkCreateGraphicsPipelines(engine::ctx.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
    {
        printf("Failed to create pipeline\n");
        abort();
    }

    QUEUE_OBJ_DESTROY(pipeline);
    QUEUE_OBJ_DESTROY(layout);
    return pipeline;
}

ComputePipelineBuilder& ComputePipelineBuilder::set_shader_module(VkShaderModule module)
{
    shaderModule = module;
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::set_descriptor_set_layouts(std::initializer_list<VkDescriptorSetLayout> dsLayouts)
{
    descriptorSetLayouts = dsLayouts;
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::set_push_constant_ranges(std::initializer_list<VkPushConstantRange> ranges)
{
    pushConstantRanges = ranges;
    return *this;
}

VkPipeline ComputePipelineBuilder::build()
{
    VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.pSetLayouts = descriptorSetLayouts.data();
	layoutInfo.setLayoutCount = descriptorSetLayouts.size();
    layoutInfo.pPushConstantRanges = pushConstantRanges.data();
    layoutInfo.pushConstantRangeCount = pushConstantRanges.size();
	VK_CHECK(vkCreatePipelineLayout(engine::ctx.device, &layoutInfo, nullptr, &layout));

    VkPipelineShaderStageCreateInfo stageInfo{};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.pNext = nullptr;
	stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.module = shaderModule;
	stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.stage = stageInfo;

	VK_CHECK(vkCreateComputePipelines(engine::ctx.device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline));
    
    QUEUE_OBJ_DESTROY(pipeline);
    QUEUE_OBJ_DESTROY(layout);
    return pipeline;
}
