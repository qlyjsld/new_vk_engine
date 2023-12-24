#include "vk_pipeline_builder.h"

#include <iostream>
#include <vulkan/vulkan_core.h>

void PipelineBuilder::customize(VkExtent2D window_extent,
                                vertex_input_description *description)
{
    _viewport.x = 0.f;
    _viewport.y = 0.f;
    _viewport.width = window_extent.width;
    _viewport.height = window_extent.height;
    _viewport.minDepth = 0.f;
    _viewport.maxDepth = 1.f;

    _scissor.offset = VkOffset2D{0, 0};
    _scissor.extent = window_extent;

    _vertex_input_state_info.vertexBindingDescriptionCount = description->bindings.size();
    _vertex_input_state_info.pVertexBindingDescriptions = description->bindings.data();
    _vertex_input_state_info.vertexAttributeDescriptionCount =
        description->attributes.size();
    _vertex_input_state_info.pVertexAttributeDescriptions =
        description->attributes.data();
}

void PipelineBuilder::build(VkDevice device, VkFormat *format, VkFormat depth_format)
{
    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    // viewport_state_info.flags = ;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &_viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &_scissor;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info = {};
    color_blend_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.pNext = nullptr;
    // color_blend_state_info.flags = ;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &_color_blend_attachment_state;
    // color_blend_state_info.blendConstants[] = ;

    VkPipelineRenderingCreateInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.pNext = nullptr;
    // rendering_info.viewMask = ;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = format;
    rendering_info.depthAttachmentFormat = depth_format;
    // rendering_info.stencilAttachmentFormat = ;

    VkGraphicsPipelineCreateInfo graphics_pipeline_info = {};
    graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.pNext = &rendering_info;
    // graphics_pipeline_info.flags = ;
    graphics_pipeline_info.stageCount = _shader_stage_infos.size();
    graphics_pipeline_info.pStages = _shader_stage_infos.data();
    graphics_pipeline_info.pVertexInputState = &_vertex_input_state_info;
    graphics_pipeline_info.pInputAssemblyState = &_input_asm_state_info;
    // graphics_pipeline_info.pTessellationState = ;
    graphics_pipeline_info.pViewportState = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &_rasterization_state_info;
    graphics_pipeline_info.pMultisampleState = &_multisample_state_info;
    graphics_pipeline_info.pDepthStencilState = &_depth_stencil_state_info;
    graphics_pipeline_info.pColorBlendState = &color_blend_state_info;
    // graphics_pipeline_info.pDynamicState = ;
    graphics_pipeline_info.layout = _pipeline_layout;
    // graphics_pipeline_info.renderPass = ;
    // graphics_pipeline_info.subpass = ;
    graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    // graphics_pipeline_info.basePipelineIndex = ;

    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_info,
                                       nullptr, &_pipeline));
}