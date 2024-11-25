#pragma once

#include <vector>
#include <volk.h>

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

    void build_layout(VkDevice device,
                      std::vector<VkDescriptorSetLayout> &layouts,
                      std::vector<VkPushConstantRange> &push_constants,
                      VkPipelineLayout *pipeline_layout);

    void build_gfx(VkDevice device, VkFormat *format, VkFormat depth_format,
                   VkPipelineLayout *pipeline_layout, VkPipeline *pipeline);

    void build_comp(VkDevice device,
                    std::vector<VkPushConstantRange> &push_constants,
                    struct cs *cs);
};
