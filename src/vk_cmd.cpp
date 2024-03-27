#include "vk_cmd.h"

#include "vk_boiler.h"

void vk_cmd::vk_img_layout_transition(VkCommandBuffer cmd_buffer, VkImage img,
                                      VkImageLayout old_layout, VkImageLayout new_layout,
                                      uint32_t family_index)
{
    VkImageSubresourceRange subresource_range =
        vk_boiler::img_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier img_mem_barrier = vk_boiler::img_mem_barrier();
    img_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_mem_barrier.pNext = nullptr;
    // img_mem_barrier.srcAccessMask = ;
    // img_mem_barrier.dstAccessMask = ;
    img_mem_barrier.oldLayout = old_layout;
    img_mem_barrier.newLayout = new_layout;
    img_mem_barrier.srcQueueFamilyIndex = family_index;
    img_mem_barrier.dstQueueFamilyIndex = family_index;
    img_mem_barrier.image = img;
    img_mem_barrier.subresourceRange = subresource_range;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &img_mem_barrier);
}