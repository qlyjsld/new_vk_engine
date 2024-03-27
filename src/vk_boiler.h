﻿#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace vk_boiler
{
VkCommandPoolCreateInfo cmd_pool_create_info(uint32_t queue_family_index);

VkCommandBufferAllocateInfo cmd_buffer_allocate_info(uint32_t cmd_buffer_count,
                                                     VkCommandPool cmd_pool);

VkFenceCreateInfo fence_create_info(bool signaled);

VkSemaphoreCreateInfo sem_create_info();

VkRenderingAttachmentInfo rendering_attachment_info(VkImageView img_view,
                                                    VkImageLayout layout,
                                                    VkClearValue clear_value);

VkRenderingInfo rendering_info(VkRenderingAttachmentInfo *color_attachments,
                               VkRenderingAttachmentInfo *depth_attachments,
                               VkExtent2D extent);

VkCommandBufferBeginInfo cmd_buffer_begin_info();

VkImageSubresourceRange img_subresource_range(VkImageAspectFlags aspect);

VkImageMemoryBarrier img_mem_barrier();

VkSubmitInfo submit_info(VkCommandBuffer *cmd_buffer, VkSemaphore *wait_sem,
                         VkSemaphore *signal_sem, VkPipelineStageFlags *flags);

VkPresentInfoKHR present_info(VkSwapchainKHR *swapchain, VkSemaphore *sem,
                              uint32_t *img_indices);

VkPipelineShaderStageCreateInfo shader_stage_create_info(VkShaderStageFlagBits stage,
                                                         VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

VkPipelineInputAssemblyStateCreateInfo
input_asm_state_create_info(VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygon_mode);

VkPipelineColorBlendAttachmentState color_blend_attachment_state();

VkPipelineMultisampleStateCreateInfo multisample_state_create_info();

VkPipelineLayoutCreateInfo
pipeline_layout_create_info(std::vector<VkDescriptorSetLayout> &layouts);

VkImageCreateInfo img_create_info(VkFormat format, VkExtent3D extent,
                                  VkImageUsageFlags usage);

VkImageViewCreateInfo img_view_create_info(VkImageAspectFlags aspect, VkImage img,
                                           VkFormat format);

VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info();

VkDescriptorPoolCreateInfo desc_pool_create_info(uint32_t pool_size_count,
                                                 VkDescriptorPoolSize *desc_pool_sizes);

VkDescriptorSetLayoutCreateInfo
desc_set_layout_create_info(uint32_t binding_count,
                            VkDescriptorSetLayoutBinding *desc_set_layout_bindings);

VkDescriptorSetAllocateInfo
desc_set_allocate_info(VkDescriptorPool desc_pool,
                       VkDescriptorSetLayout *desc_set_layout);

VkWriteDescriptorSet write_desc_set(VkDescriptorBufferInfo *desc_buffer_info,
                                    VkDescriptorSet set, VkDescriptorType type);

VkWriteDescriptorSet write_desc_set(VkDescriptorImageInfo *desc_img_info,
                                    VkDescriptorSet set, VkDescriptorType type);

VkBufferImageCopy buffer_img_copy(VkExtent3D extent);

VkSamplerCreateInfo sampler_create_info();

} // namespace vk_boiler