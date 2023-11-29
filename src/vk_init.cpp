#include "vk_init.h"
#include <cstddef>
#include <vulkan/vulkan_core.h>

VkCommandPoolCreateInfo vk_init::vk_create_cmd_pool_info(uint32_t queue_family_index)
{
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = queue_family_index;
    return cmd_pool_info;
}

VkCommandBufferAllocateInfo
vk_init::vk_create_cmd_buffer_allocate_info(uint32_t cmd_buffer_count,
                                            VkCommandPool cmd_pool)
{
    VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
    cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_allocate_info.pNext = nullptr;
    cmd_buffer_allocate_info.commandPool = cmd_pool;
    cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_allocate_info.commandBufferCount = cmd_buffer_count;
    return cmd_buffer_allocate_info;
}

VkFenceCreateInfo vk_init::vk_create_fence_info(bool signaled)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    if (signaled)
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return fence_info;
}

VkSemaphoreCreateInfo vk_init::vk_create_sem_info()
{
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.pNext = nullptr;
    return sem_info;
}

VkRenderingAttachmentInfo
vk_init::vk_create_rendering_attachment_info(VkClearValue clear_value,
                                             VkImageView image_view)
{
    VkRenderingAttachmentInfo attachment_info = {};
    attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment_info.pNext = nullptr;
    attachment_info.imageView = image_view;
    attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // attachment_info.resolveMode = ;
    // attachment_info.resolveImageView = ;
    // attachment_info.resolveImageLayout = ;
    attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.clearValue = clear_value;
    return attachment_info;
}

VkRenderingInfo
vk_init::vk_create_rendering_info(VkRenderingAttachmentInfo *color_attachments,
                                  VkExtent2D extent)
{
    VkRect2D render_area = {};
    render_area.offset = VkOffset2D{0, 0};
    render_area.extent = extent;

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.pNext = nullptr;
    rendering_info.flags = 0;
    rendering_info.renderArea = render_area;
    rendering_info.layerCount = 1;
    rendering_info.viewMask = 0;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = color_attachments;
    rendering_info.pDepthAttachment = nullptr;
    rendering_info.pStencilAttachment = nullptr;
    return rendering_info;
}

VkCommandBufferBeginInfo vk_init::vk_create_cmd_buffer_begin_info()
{
    VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
    cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buffer_begin_info.pNext = nullptr;
    cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmd_buffer_begin_info.pInheritanceInfo = nullptr;
    return cmd_buffer_begin_info;
}

VkImageSubresourceRange vk_init::vk_create_subresource_range()
{
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    return subresource_range;
}

VkImageMemoryBarrier vk_init::vk_create_img_mem_barrier()
{
    VkImageMemoryBarrier img_mem_barrier = {};
    return img_mem_barrier;
}

VkSubmitInfo vk_init::vk_create_submit_info(VkCommandBuffer *cmd_buffer,
                                            VkSemaphore *wait_sem,
                                            VkSemaphore *signal_sem,
                                            VkPipelineStageFlags *pipeline_stage_flags)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_sem;
    submit_info.pWaitDstStageMask = pipeline_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_sem;
    return submit_info;
}

VkPresentInfoKHR vk_init::vk_create_present_info(VkSwapchainKHR *swapchain,
                                                 VkSemaphore *sem,
                                                 uint32_t *image_indices)
{
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = sem;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchain;
    present_info.pImageIndices = image_indices;
    present_info.pResults = nullptr;
    return present_info;
}

VkPipelineShaderStageCreateInfo
vk_init::vk_create_shader_stage_info(VkShaderStageFlagBits stage,
                                     VkShaderModule shader_module)
{
    VkPipelineShaderStageCreateInfo shader_stage_info = {};
    shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.pNext = nullptr;
    // shader_stage_info.flags = ;
    shader_stage_info.stage = stage;
    shader_stage_info.module = shader_module;
    shader_stage_info.pName = "main";
    // shader_stage_info.pSpecializationInfo = ;
    return shader_stage_info;
}

VkPipelineVertexInputStateCreateInfo vk_init::vk_create_vertex_input_state_info()
{
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pNext = nullptr;
    // vertex_input_state_info.flags = ;
    vertex_input_state_info.vertexBindingDescriptionCount = 0;
    vertex_input_state_info.pVertexBindingDescriptions = nullptr;
    vertex_input_state_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_info.pVertexAttributeDescriptions = nullptr;
    return vertex_input_state_info;
}

VkPipelineInputAssemblyStateCreateInfo
vk_init::vk_create_input_asm_state_info(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo input_asm_state_info = {};
    input_asm_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_state_info.pNext = nullptr;
    // input_asm_state_info.flags = ;
    input_asm_state_info.topology = topology;
    input_asm_state_info.primitiveRestartEnable = VK_FALSE;
    return input_asm_state_info;
}

VkPipelineRasterizationStateCreateInfo
vk_init::vk_create_rasterization_state_info(VkPolygonMode polygon_mode)
{
    VkPipelineRasterizationStateCreateInfo rasterization_state_info = {};
    rasterization_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.pNext = nullptr;
    // rasterization_state_info.flags = ;
    rasterization_state_info.depthClampEnable = VK_FALSE;
    rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_info.polygonMode = polygon_mode;
    rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = VK_FALSE;
    // rasterization_state_info.depthBiasConstantFactor = ;
    // rasterization_state_info.depthBiasClamp = ;
    // rasterization_state_info.depthBiasSlopeFactor = ;
    rasterization_state_info.lineWidth = 1.f;
    return rasterization_state_info;
}

VkPipelineColorBlendAttachmentState vk_init::vk_create_color_blend_attachment_state()
{
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    // color_blend_attachment_state.srcColorBlendFactor = ;
    // color_blend_attachment_state.dstColorBlendFactor = ;
    // color_blend_attachment_state.colorBlendOp = ;
    // color_blend_attachment_state.srcAlphaBlendFactor = ;
    // color_blend_attachment_state.dstAlphaBlendFactor = ;
    // color_blend_attachment_state.alphaBlendOp = ;
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    return color_blend_attachment_state;
}

VkPipelineMultisampleStateCreateInfo vk_init::vk_create_multisample_state_info()
{
    VkPipelineMultisampleStateCreateInfo multisample_state_info = {};
    multisample_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.pNext = nullptr;
    // multisample_state_info.flags = ;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    // multisample_state_info.minSampleShading = ;
    multisample_state_info.pSampleMask = nullptr;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;
    return multisample_state_info;
}

VkPipelineLayoutCreateInfo vk_init::vk_create_pipeline_layout_info()
{
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    // pipeline_layout_info.flags = ;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    return pipeline_layout_info;
}