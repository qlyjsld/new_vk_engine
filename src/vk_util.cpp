#include "vk_engine.h"

#include <fstream>
#include <iostream>

#include "vk_boiler.h"
#include "vk_type.h"

void vk_engine::immediate_draw(std::function<void(VkCommandBuffer cmd)> &&fs,
                               VkQueue queue)
{
    /* prepare command buffer */
    VkCommandBufferBeginInfo cbuffer_begin_info =
        vk_boiler::cbuffer_begin_info();

    /* begin command buffer recording */
    VK_CHECK(vkBeginCommandBuffer(_immed_context.cbuffer, &cbuffer_begin_info));

    fs(_immed_context.cbuffer);

    VK_CHECK(vkEndCommandBuffer(_immed_context.cbuffer));

    VkSubmitInfo submit_info = vk_boiler::submit_info(
        &_immed_context.cbuffer, nullptr, nullptr, nullptr);

    submit_info.waitSemaphoreCount = 0;
    submit_info.signalSemaphoreCount = 0;

    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, _immed_context.fence));
    VK_CHECK(vkWaitForFences(_device, 1, &_immed_context.fence, VK_TRUE,
                             UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &_immed_context.fence));
}

VkShaderModule vk_engine::load_shader_module(const char *filename)
{
    std::ifstream f(filename, std::ios::ate | std::ios::binary);
    VkShaderModule shader_module = VK_NULL_HANDLE;

    if (!f.is_open()) {
        std::cerr << "shader: " << filename << " not exist" << std::endl;
        return shader_module;
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

    VK_CHECK(vkCreateShaderModule(_device, &shader_module_info, nullptr,
                                  &shader_module));

    deletion_queue.push_back(
        [=]() { vkDestroyShaderModule(_device, shader_module, nullptr); });

    return shader_module;
}

void vk_engine::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                              VmaAllocationCreateFlags flags,
                              allocated_buffer *buffer)
{
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vma_allocation_info,
                             &buffer->buffer, &buffer->allocation, nullptr));

    deletion_queue.push_back([=]() {
        vmaDestroyBuffer(_allocator, buffer->buffer, buffer->allocation);
    });
}

void vk_engine::create_img(VkFormat format, VkExtent3D extent,
                           VkImageAspectFlags aspect, VkImageUsageFlags usage,
                           VmaAllocationCreateFlags flags, allocated_img *img)
{
    VkImageCreateInfo img_info =
        vk_boiler::img_create_info(format, extent, usage);

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    img->format = format;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &vma_allocation_info,
                            &img->img, &img->allocation, nullptr));

    deletion_queue.push_back(
        [=]() { vmaDestroyImage(_allocator, img->img, img->allocation); });

    VkImageViewCreateInfo img_view_info =
        vk_boiler::img_view_create_info(aspect, img->img, extent, format);

    VK_CHECK(
        vkCreateImageView(_device, &img_view_info, nullptr, &img->img_view));

    deletion_queue.push_back(
        [=]() { vkDestroyImageView(_device, img->img_view, nullptr); });
}

size_t vk_engine::pad_uniform_buffer_size(size_t original_size)
{
    size_t aligned_size = original_size;
    if (_min_buffer_alignment > 0)
        aligned_size = (aligned_size + _min_buffer_alignment - 1) &
                       ~(_min_buffer_alignment - 1);
    return aligned_size;
}
