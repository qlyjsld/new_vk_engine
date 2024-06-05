#pragma once

#include <volk.h>

namespace vk_cmd
{
void vk_img_layout_transition(VkCommandBuffer cbuffer, VkImage img,
                              VkImageLayout old_layout, VkImageLayout new_layout,
                              uint32_t family_index);

void vk_img_copy(VkCommandBuffer cbuffer, VkExtent3D extent, VkImage src, VkImage dst);
} // namespace vk_cmd