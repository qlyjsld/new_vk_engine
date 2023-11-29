#pragma once

#include "vk_type.h"
#include <vulkan/vulkan_core.h>

namespace vk_init
{
VkCommandPoolCreateInfo vk_create_cmd_pool_info(uint32_t queue_family_index);

VkCommandBufferAllocateInfo vk_create_cmd_buffer_allocate_info(uint32_t cmd_buffer_count,
                                                               VkCommandPool cmd_pool);

VkFenceCreateInfo vk_create_fence_info(bool signaled);

VkSemaphoreCreateInfo vk_create_sem_info();

VkRenderingAttachmentInfo vk_create_rendering_attachment_info(VkClearValue clear_value,
                                                              VkImageView image_view);

VkRenderingInfo vk_create_rendering_info(VkRenderingAttachmentInfo *color_attachments,
                                         VkExtent2D extent);

VkCommandBufferBeginInfo vk_create_cmd_buffer_begin_info();

VkImageSubresourceRange vk_create_subresource_range();

VkImageMemoryBarrier vk_create_img_mem_barrier();

VkSubmitInfo vk_create_submit_info(VkCommandBuffer *cmd_buffer, VkSemaphore *wait_sem,
                                   VkSemaphore *signal_sem, VkPipelineStageFlags *flags);

VkPresentInfoKHR vk_create_present_info(VkSwapchainKHR *swapchain, VkSemaphore *sem,
                                        uint32_t *image_indices);

VkPipelineShaderStageCreateInfo vk_create_shader_stage_info(VkShaderStageFlagBits stage,
                                                            VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo vk_create_vertex_input_state_info();

VkPipelineInputAssemblyStateCreateInfo
vk_create_input_asm_state_info(VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo
vk_create_rasterization_state_info(VkPolygonMode polygon_mode);

VkPipelineColorBlendAttachmentState vk_create_color_blend_attachment_state();

VkPipelineMultisampleStateCreateInfo vk_create_multisample_state_info();

VkPipelineLayoutCreateInfo vk_create_pipeline_layout_info();

} // namespace vk_init
