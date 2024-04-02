#pragma once

#include <vulkan/vulkan.h>

namespace vk_cmd
{
void vk_img_layout_transition(VkCommandBuffer cmd_buffer, VkImage img,
                              VkImageLayout old_layout, VkImageLayout new_layout,
                              uint32_t family_index);

void vk_img_copy(VkCommandBuffer cmd_buffer, VkExtent3D extent, VkImage src, VkImage dst);
} // namespace vk_cmd