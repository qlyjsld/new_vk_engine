﻿#pragma once

#include <vector>
#include <volk.h>

#include "vk_mesh.h"

namespace vk_boiler
{
inline VkCommandPoolCreateInfo cpool_create_info(uint32_t index)
{
    VkCommandPoolCreateInfo cpool_info = {};
    cpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpool_info.pNext = nullptr;
    cpool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                       VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpool_info.queueFamilyIndex = index;
    return cpool_info;
}

inline VkCommandBufferAllocateInfo cbuffer_allocate_info(uint32_t cbuffer_count,
                                                         VkCommandPool cpool)
{
    VkCommandBufferAllocateInfo cbuffer_allocate_info = {};
    cbuffer_allocate_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbuffer_allocate_info.pNext = nullptr;
    cbuffer_allocate_info.commandPool = cpool;
    cbuffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbuffer_allocate_info.commandBufferCount = cbuffer_count;
    return cbuffer_allocate_info;
}

inline VkFenceCreateInfo fence_create_info(bool signaled)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    if (signaled)
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return fence_info;
}

inline VkSemaphoreCreateInfo sem_create_info()
{
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.pNext = nullptr;
    return sem_info;
}

inline VkRenderingAttachmentInfo
rendering_attachment_info(VkImageView img_view, VkImageLayout layout,
                          bool clear, VkClearValue clear_value)
{
    VkRenderingAttachmentInfo attachment_info = {};
    attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment_info.pNext = nullptr;
    attachment_info.imageView = img_view;
    attachment_info.imageLayout = layout;
    // attachment_info.resolveMode = ;
    // attachment_info.resolveImageView = ;
    // attachment_info.resolveImageLayout = ;
    if (clear)
        attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    else
        attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.clearValue = clear_value;
    return attachment_info;
}

inline VkRenderingInfo
rendering_info(VkRenderingAttachmentInfo *color_attachments,
               VkRenderingAttachmentInfo *depth_attachments, VkExtent2D extent)
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
    rendering_info.pDepthAttachment = depth_attachments;
    rendering_info.pStencilAttachment = nullptr;
    return rendering_info;
}

inline VkCommandBufferBeginInfo cbuffer_begin_info()
{
    VkCommandBufferBeginInfo cbuffer_begin_info = {};
    cbuffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbuffer_begin_info.pNext = nullptr;
    cbuffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cbuffer_begin_info.pInheritanceInfo = nullptr;
    return cbuffer_begin_info;
}

inline VkSubmitInfo submit_info(VkCommandBuffer *cbuffer, VkSemaphore *wait_sem,
                                VkSemaphore *signal_sem,
                                VkPipelineStageFlags *flags)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_sem;
    submit_info.pWaitDstStageMask = flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cbuffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_sem;
    return submit_info;
}

inline VkPresentInfoKHR present_info(VkSwapchainKHR *swapchain,
                                     VkSemaphore *sem, uint32_t *img_indices)
{
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = sem;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchain;
    present_info.pImageIndices = img_indices;
    present_info.pResults = nullptr;
    return present_info;
}

inline VkImageSubresourceRange img_subresource_range(VkImageAspectFlags aspect)
{
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = aspect;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    return subresource_range;
}

inline VkImageMemoryBarrier img_mem_barrier()
{
    VkImageMemoryBarrier img_mem_barrier = {};
    return img_mem_barrier;
}

inline VkPipelineShaderStageCreateInfo
shader_stage_create_info(VkShaderStageFlagBits stage,
                         VkShaderModule shader_module)
{
    VkPipelineShaderStageCreateInfo shader_stage_info = {};
    shader_stage_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.pNext = nullptr;
    // shader_stage_info.flags = ;
    shader_stage_info.stage = stage;
    shader_stage_info.module = shader_module;
    shader_stage_info.pName = "main";
    // shader_stage_info.pSpecializationInfo = ;
    return shader_stage_info;
}

inline VkPipelineVertexInputStateCreateInfo
vertex_input_state_create_info(vertex_input_description *description)
{
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pNext = nullptr;
    // vertex_input_state_info.flags = ;
    vertex_input_state_info.vertexBindingDescriptionCount =
        description->bindings.size();
    vertex_input_state_info.pVertexBindingDescriptions =
        description->bindings.data();
    vertex_input_state_info.vertexAttributeDescriptionCount =
        description->attributes.size();
    vertex_input_state_info.pVertexAttributeDescriptions =
        description->attributes.data();
    return vertex_input_state_info;
}

inline VkPipelineInputAssemblyStateCreateInfo
input_asm_state_create_info(VkPrimitiveTopology topology)
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

inline VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygon_mode)
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

inline VkPipelineColorBlendAttachmentState color_blend_attachment_state()
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
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return color_blend_attachment_state;
}

inline VkPipelineMultisampleStateCreateInfo multisample_state_create_info()
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

inline VkPipelineLayoutCreateInfo
pipeline_layout_create_info(std::vector<VkDescriptorSetLayout> &layouts,
                            std::vector<VkPushConstantRange> &push_constants)
{
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    // pipeline_layout_info.flags = ;
    pipeline_layout_info.setLayoutCount = layouts.size();
    pipeline_layout_info.pSetLayouts = layouts.data();
    pipeline_layout_info.pushConstantRangeCount = push_constants.size();
    pipeline_layout_info.pPushConstantRanges = push_constants.data();
    return pipeline_layout_info;
}

inline VkImageCreateInfo img_create_info(VkFormat format, VkExtent3D extent,
                                         VkImageUsageFlags usage)
{

    VkImageCreateInfo img_info = {};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.pNext = nullptr;
    // img_info.flags = ;
    img_info.imageType =
        extent.depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    img_info.format = format;
    img_info.extent = extent;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = usage;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // img_info.queueFamilyIndexCount = ;
    // img_info.pQueueFamilyIndices = ;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return img_info;
}

inline VkImageViewCreateInfo img_view_create_info(VkImageAspectFlags aspect,
                                                  VkImage img,
                                                  VkExtent3D extent,
                                                  VkFormat format)
{
    VkImageSubresourceRange subresource_range = img_subresource_range(aspect);
    VkImageViewCreateInfo img_view_info = {};
    img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.pNext = nullptr;
    // img_view_info.flags = ;
    img_view_info.image = img;
    img_view_info.viewType =
        extent.depth == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    img_view_info.format = format;
    // img_view_info.components = ;
    img_view_info.subresourceRange = subresource_range;
    return img_view_info;
}

inline VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info()
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
    depth_stencil_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    // depth_stencil_state_info.flags = ;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;
    // depth_stencil_state_info.front = ;
    // depth_stencil_state_info.back = ;
    // depth_stencil_state_info.minDepthBounds = ;
    // depth_stencil_state_info.maxDepthBounds = ;
    return depth_stencil_state_info;
}

inline VkDescriptorPoolCreateInfo
descriptor_pool_create_info(uint32_t pool_size_count,
                            VkDescriptorPoolSize *pool_sizes)
{
    VkDescriptorPoolCreateInfo descriptor_pool_info = {};
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags =
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_info.maxSets = 1024;
    descriptor_pool_info.poolSizeCount = pool_size_count;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    return descriptor_pool_info;
}

inline VkDescriptorSetLayoutCreateInfo
descriptor_set_layout_create_info(std::vector<VkDescriptorType> types,
                                  VkShaderStageFlags stage)
{
    std::vector<VkDescriptorSetLayoutBinding> *bindings =
        new std::vector<VkDescriptorSetLayoutBinding>;

    for (uint32_t i = 0; i < types.size(); ++i) {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = i;
        binding.descriptorType = types[i];
        binding.descriptorCount = 1;
        binding.stageFlags = stage;
        bindings->push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
    descriptor_set_layout_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    // descriptor_set_layout_info.flags = ;
    descriptor_set_layout_info.bindingCount = bindings->size();
    descriptor_set_layout_info.pBindings = bindings->data();

    deletion_queue.push_back([=]() { delete bindings; });

    return descriptor_set_layout_info;
}

inline VkDescriptorSetAllocateInfo
descriptor_set_allocate_info(VkDescriptorPool pool,
                             VkDescriptorSetLayout *layouts)
{
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = layouts;
    return descriptor_set_allocate_info;
}

inline VkWriteDescriptorSet
write_descriptor_set(VkDescriptorBufferInfo *buffer_info, VkDescriptorSet set,
                     uint32_t binding, VkDescriptorType type)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = set;
    write_set.dstBinding = binding;
    write_set.descriptorCount = 1;
    write_set.descriptorType = type;
    write_set.pBufferInfo = buffer_info;
    return write_set;
}

inline VkWriteDescriptorSet
write_descriptor_set(VkDescriptorImageInfo *img_info, VkDescriptorSet set,
                     uint32_t binding, VkDescriptorType type)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = set;
    write_set.dstBinding = binding;
    write_set.descriptorCount = 1;
    write_set.descriptorType = type;
    write_set.pImageInfo = img_info;
    return write_set;
}

inline VkBufferImageCopy buffer_img_copy(VkExtent3D extent)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = VkOffset3D{0, 0, 0};
    region.imageExtent = extent;
    return region;
}

inline VkSamplerCreateInfo sampler_create_info()
{
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    return sampler_info;
}

inline VkViewport viewport(VkExtent2D extent)
{
    VkViewport viewport = {};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    return viewport;
}

inline VkRect2D scissor(VkExtent2D extent)
{
    VkRect2D scissor = {};
    scissor.offset = VkOffset2D{0, 0};
    scissor.extent = extent;
    return scissor;
}
} // namespace vk_boiler
