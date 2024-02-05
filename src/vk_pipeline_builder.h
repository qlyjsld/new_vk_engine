#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "vk_mesh.h"

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
    VkPipelineDepthStencilStateCreateInfo _depth_stencil_state_info;
    VkPipelineLayout _pipeline_layout;
    VkPipeline _pipeline;

    void customize(VkExtent2D window_extent, vertex_input_description *description);
    void build_gfx(VkDevice device, VkFormat *format, VkFormat depth_format);
    void build_comp(VkDevice device);
    VkPipeline value() { return _pipeline; };
};
