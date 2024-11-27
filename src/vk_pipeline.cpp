#include "vk_pipeline.h"

#include <volk.h>

#include "vk_boiler.h"
#include "vk_comp.h"
#include "vk_type.h"

VkPipelineLayout
PipelineBuilder::build_layout(VkDevice device,
                              std::vector<VkDescriptorSetLayout> &layouts,
                              std::vector<VkPushConstantRange> &push_constants)
{
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo pipeline_layout_info =
        vk_boiler::pipeline_layout_create_info(layouts, push_constants);

    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr,
                                    &pipeline_layout));

    deletion_queue.push_back(
        [=]() { vkDestroyPipelineLayout(device, pipeline_layout, nullptr); });

    return pipeline_layout;
}

VkPipeline PipelineBuilder::build_gfx(VkDevice device, VkFormat *format,
                                      VkFormat depth_format,
                                      VkPipelineLayout pipeline_layout)
{
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
    graphics_pipeline_info.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
    graphics_pipeline_info.layout = pipeline_layout;
    // graphics_pipeline_info.renderPass = ;
    // graphics_pipeline_info.subpass = ;
    graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    // graphics_pipeline_info.basePipelineIndex = ;

    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                       &graphics_pipeline_info, nullptr,
                                       &pipeline));

    deletion_queue.push_back(
        [=]() { vkDestroyPipeline(device, pipeline, nullptr); });

    return pipeline;
}

void PipelineBuilder::build_comp(
    VkDevice device, std::vector<VkPushConstantRange> &push_constants, cs *cs)
{
    std::vector<VkDescriptorSetLayout> layouts = {cs->layout};
    cs->pipeline_layout = build_layout(device, layouts, push_constants);

    VkComputePipelineCreateInfo comp_pipeline_info = {};
    comp_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    comp_pipeline_info.pNext = nullptr;
    // comp_pipeline_info.flags = ;
    comp_pipeline_info.stage = _shader_stage_infos[0];
    comp_pipeline_info.layout = cs->pipeline_layout;
    comp_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    // comp_pipeline_info.basePipelineIndex = ;

    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                      &comp_pipeline_info, nullptr,
                                      &cs->pipeline));

    VkPipeline pipeline = cs->pipeline;
    deletion_queue.push_back(
        [=]() { vkDestroyPipeline(device, pipeline, nullptr); });
}
