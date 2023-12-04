#pragma once

#include "vk_mesh.h"
#include <vector>
#include <vulkan/vulkan_core.h>

class PipelineBuilder
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stage_infos;
    VkPipelineVertexInputStateCreateInfo _vertex_input_state_info;
    VkPipelineInputAssemblyStateCreateInfo _input_asm_state_info;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterization_state_info;
    VkPipelineColorBlendAttachmentState _color_blend_attachment_state;
    VkPipelineMultisampleStateCreateInfo _multisample_state_info;
    VkPipelineLayout _pipeline_layout;
    VkPipeline _pipeline;

    void customize(VkExtent2D window_extent, vertex_input_description *description);
    void build(VkDevice device, VkFormat *format);
    VkPipeline value() { return _pipeline; };
};
