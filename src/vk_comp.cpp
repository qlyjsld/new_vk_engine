#include "vk_comp.h"

#include <fstream>
#include <iostream>

void comp_allocator::create_new_pool()
{
    VkDescriptorPool new_pool;

    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256},
    };

    VkDescriptorPoolCreateInfo pool_info =
        vk_boiler::descriptor_pool_create_info(pool_sizes.size(), pool_sizes.data());

    vkCreateDescriptorPool(device, &pool_info, nullptr, &new_pool);

    deletion_queue.push_back(
        [=]() { vkDestroyDescriptorPool(device, new_pool, nullptr); });

    pools.push_back(new_pool);
}

void comp_allocator::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VmaAllocationCreateFlags flags, std::string name)
{
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_allocation_info,
                             &buffers[name].buffer, &buffers[name].allocation, nullptr));

    deletion_queue.push_back([=]() {
        vmaDestroyBuffer(allocator, buffers[name].buffer, buffers[name].allocation);
    });
}

void comp_allocator::create_img(VkFormat format, VkExtent3D extent,
                                VkImageAspectFlags aspect, VkImageUsageFlags usage,
                                VmaAllocationCreateFlags flags, std::string name)
{
    VkImageCreateInfo img_info = vk_boiler::img_create_info(format, extent, usage);

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    imgs[name].format = format;

    VK_CHECK(vmaCreateImage(allocator, &img_info, &vma_allocation_info, &imgs[name].img,
                            &imgs[name].allocation, nullptr));

    deletion_queue.push_back(
        [=]() { vmaDestroyImage(allocator, imgs[name].img, imgs[name].allocation); });

    VkImageViewCreateInfo img_view_info =
        vk_boiler::img_view_create_info(aspect, imgs[name].img, format);

    VK_CHECK(vkCreateImageView(device, &img_view_info, nullptr, &imgs[name].img_view));

    deletion_queue.push_back(
        [=]() { vkDestroyImageView(device, imgs[name].img_view, nullptr); });
}

void comp_allocator::allocate_descriptor_set(std::vector<VkDescriptorType> types,
                                             VkDescriptorSetLayout *layout,
                                             VkDescriptorSet *set)
{
    VkDescriptorSetLayoutCreateInfo layout_info =
        vk_boiler::descriptor_set_layout_create_info(types, VK_SHADER_STAGE_COMPUTE_BIT);

    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, layout));

    deletion_queue.push_back(
        [=]() { vkDestroyDescriptorSetLayout(device, *layout, nullptr); });

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
        vk_boiler::descriptor_set_allocate_info(get_pool(), layout);

    VK_CHECK(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, set));
};

bool cs::load_shader_module(const char *filename)
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

    VK_CHECK(vkCreateShaderModule(device, &shader_module_info, nullptr, &module));

    deletion_queue.push_back([=]() { vkDestroyShaderModule(device, module, nullptr); });

    return true;
}

size_t cs::pad_uniform_buffer_size(size_t original_size)
{
    size_t aligned_size = original_size;
    if (min_uniform_buffer_offset_alignment > 0)
        aligned_size = (aligned_size + min_uniform_buffer_offset_alignment - 1) &
                       ~(min_uniform_buffer_offset_alignment - 1);
    return aligned_size;
}