#pragma once

#include "vk_type.h"

namespace vk_cmd
{
void vk_img_layout_transition(VkCommandBuffer cmd_buffer, VkImage img,
                              VkImageLayout old_layout, VkImageLayout new_layout,
                              uint32_t family_index);
}