#pragma once
#include <initializer_list>
#include <vulkan/vulkan_core.h>
#include "types.hpp"

//subparameters
struct ShaderStage {
    VkShaderStageFlagBits            stage;
    VkShaderModule                   module;
    VkPipelineShaderStageCreateFlags flags = 0;
};

struct Blend {
    VkBlendFactor src;
    VkBlendFactor dst;
    VkBlendOp     op = VK_BLEND_OP_ADD;
};

VkPipelineColorBlendAttachmentState color_blend(Blend color, Blend alpha);
VkPipelineColorBlendAttachmentState color_blend(); //disabled color blend

//#TODO: maake it a inheritable class but who cares about that rn
struct ComputePipelineBuilder {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange>   pushConstantRanges;
    VkShaderModule                     shaderModule;
    VkPipeline                         pipeline;
    VkPipelineLayout                   layout;
    ComputePipelineBuilder&            set_shader_module(VkShaderModule module);
    ComputePipelineBuilder&            set_descriptor_set_layouts(std::initializer_list<VkDescriptorSetLayout> dsLayouts);
    ComputePipelineBuilder&            set_push_constant_ranges(std::initializer_list<VkPushConstantRange> ranges);
    VkPipeline                         build();
};

// VkGraphicsPipelineCreateInfo proxy
struct GraphicsPipelineBuilder {
    std::vector<VkDescriptorSetLayout>               descriptorSetLayouts;
    std::vector<VkPushConstantRange>                 pushConstantRanges;
    spock::Framebuffer                              framebuffer;
    VkPipelineCreateFlags                            flags = 0;
    std::vector<VkPipelineShaderStageCreateInfo>     stages;
    VkPipelineVertexInputStateCreateInfo             vertexInputState   = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo           inputAssemblyState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    VkPipelineTessellationStateCreateInfo            tessellationState  = {.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    VkPipelineViewportStateCreateInfo                viewportState      = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    VkPipelineRasterizationStateCreateInfo           rasterizationState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    VkPipelineMultisampleStateCreateInfo             multisampleState   = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    VkPipelineDepthStencilStateCreateInfo            depthStencilState  = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    std::vector<VkDynamicState>                      dynamicStates;

    VkPipelineLayout                                 layout = {};
    //unused
    /*
    VkRenderPass                                     renderPass;
    uint32_t                                         subpass;
    VkPipeline                                       basePipelineHandle;
    int32_t                                          basePipelineIndex;
    */

    GraphicsPipelineBuilder& set_descriptor_set_layouts(std::initializer_list<VkDescriptorSetLayout> dsLayouts);
    GraphicsPipelineBuilder& set_push_constant_ranges(std::initializer_list<VkPushConstantRange> ranges);
    GraphicsPipelineBuilder& set_framebuffer(spock::Framebuffer framebuffer);
    GraphicsPipelineBuilder& set_shader_stages(std::initializer_list<ShaderStage> shaderStages);
    GraphicsPipelineBuilder& set_viewport_state(uint32_t viewportCount = 1, uint32_t scissorCount = 1, VkViewport* viewports = VK_NULL_HANDLE, VkRect2D* scissors = VK_NULL_HANDLE);
    GraphicsPipelineBuilder& set_color_blend_states(std::initializer_list<VkPipelineColorBlendAttachmentState> attachments);
    GraphicsPipelineBuilder& set_input_assembly_state(VkPrimitiveTopology topology, bool primitiveRestartEnable = false);
    GraphicsPipelineBuilder& set_rasterization_state(VkPolygonMode polygonMode, float lineWidth, VkCullModeFlags cullMode, VkFrontFace frontFace);
    GraphicsPipelineBuilder& set_multisample_state();
    GraphicsPipelineBuilder& set_multisample_state(int rasterizationSamples, bool alphaToCoverageEnable = false, bool alphaToOneEnable = false);
    GraphicsPipelineBuilder& set_depth_stencil_state(VkCompareOp compareOp, bool writeable, VkStencilOpState front, VkStencilOpState back, float minDepthBounds,
                                                     float maxDepthBounds);
    GraphicsPipelineBuilder& set_depth_stencil_state(VkCompareOp compareOp, bool writeable, VkStencilOpState front, VkStencilOpState back);
    GraphicsPipelineBuilder& set_stencil_state(VkStencilOpState front, VkStencilOpState back);
    GraphicsPipelineBuilder& set_depth_state(VkCompareOp compareOp, bool writeable);
    GraphicsPipelineBuilder& set_depth_state(VkCompareOp compareOp, bool writeable, float minDepthBounds, float maxDepthBounds);
    GraphicsPipelineBuilder& set_rendering(VkCompareOp compareOp, bool writeable, float minDepthBounds, float maxDepthBounds);
    GraphicsPipelineBuilder& set_dynamic_states(std::initializer_list<VkDynamicState> states);
    //apparently not needed
    //VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state(VkCompareOp compareOp, bool writeable, float minDepthBounds, float maxDepthBounds);

    VkPipeline pipeline;

    VkPipeline build();
};
