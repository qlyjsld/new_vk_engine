#include "vk_engine.h"

#include <fstream>
#include <iostream>

#include "vk_boiler.h"
#include "vk_type.h"

void vk_engine::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&fs)
{
    /* prepare command buffer */
    VkCommandBufferBeginInfo cmd_buffer_begin_info = vk_boiler::cmd_buffer_begin_info();

    /* begin command buffer recording */
    VK_CHECK(vkBeginCommandBuffer(_upload_context.cmd_buffer, &cmd_buffer_begin_info));

    fs(_upload_context.cmd_buffer);

    VK_CHECK(vkEndCommandBuffer(_upload_context.cmd_buffer));

    VkSubmitInfo submit_info =
        vk_boiler::submit_info(&_upload_context.cmd_buffer, nullptr, nullptr, nullptr);

    submit_info.waitSemaphoreCount = 0;
    submit_info.signalSemaphoreCount = 0;

    VK_CHECK(vkQueueSubmit(_transfer_queue, 1, &submit_info, _upload_context.fence));
    VK_CHECK(vkWaitForFences(_device, 1, &_upload_context.fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &_upload_context.fence));
}

bool vk_engine::load_shader_module(const char *filename, VkShaderModule *shader_module)
{
    std::ifstream f(filename, std::ios::ate | std::ios::binary);

    if (!f.is_open()) {
        std::cerr << "shader: " << filename << " not exist" << std::endl;
        return false;
    }

    size_t size = f.tellg();
    std::vector<uint32_t> buffer(size / sizeof(uint32_t));

    f.seekg(0);
    f.read((char *)buffer.data(), size);
    f.close();

    VkShaderModuleCreateInfo shader_module_info = {};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.pNext = nullptr;
    shader_module_info.codeSize = buffer.size() * sizeof(uint32_t);
    shader_module_info.pCode = buffer.data();

    VK_CHECK(vkCreateShaderModule(_device, &shader_module_info, nullptr, shader_module));

    _deletion_queue.push_back(
        [=]() { vkDestroyShaderModule(_device, *shader_module, nullptr); });

    return true;
}

allocated_buffer vk_engine::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                          VmaAllocationCreateFlags flags)
{
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    allocated_buffer buffer;

    VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vma_allocation_info,
                             &buffer.buffer, &buffer.allocation, nullptr));

    return buffer;
}

allocated_img vk_engine::create_img(VkFormat format, VkExtent3D extent,
                                    VkImageAspectFlags aspect, VkImageUsageFlags usage,
                                    VmaAllocationCreateFlags flags)
{
    VkImageCreateInfo img_info = vk_boiler::img_create_info(format, extent, usage);

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    allocated_img img;
    img.format = format;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &vma_allocation_info, &img.img,
                            &img.allocation, nullptr));

    VkImageViewCreateInfo img_view_info =
        vk_boiler::img_view_create_info(aspect, img.img, format);

    VK_CHECK(vkCreateImageView(_device, &img_view_info, nullptr, &img.img_view));

    return img;
}

size_t vk_engine::pad_uniform_buffer_size(size_t original_size)
{
    size_t aligned_size = original_size;
    if (_minUniformBufferOffsetAlignment > 0)
        aligned_size = (aligned_size + _minUniformBufferOffsetAlignment - 1) &
                       ~(_minUniformBufferOffsetAlignment - 1);
    return aligned_size;
}
