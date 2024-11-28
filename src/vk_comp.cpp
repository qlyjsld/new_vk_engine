#include "vk_comp.h"

#include <fstream>
#include <iostream>

#include "vk_boiler.h"

void comp_allocator::init()
{
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16},
    };

    VkDescriptorPoolCreateInfo pool_info =
        vk_boiler::descriptor_pool_create_info(pool_sizes.size(),
                                               pool_sizes.data());

    vkCreateDescriptorPool(device, &pool_info, nullptr, &comp_descriptor_pool);

    deletion_queue.push_back([=]() {
        vkDestroyDescriptorPool(device, comp_descriptor_pool, nullptr);
    });
}

uint32_t comp_allocator::create_buffer(VkDeviceSize size,
                                       VkBufferUsageFlags usage,
                                       VmaAllocationCreateFlags flags,
                                       std::string name)
{
    allocated_buffer buffer;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateBuffer(vma_allocator, &buffer_info, &vma_allocation_info,
                             &buffer.buffer, &buffer.allocation, nullptr));

    buffer.size = size;

    deletion_queue.push_back([=]() {
        vmaDestroyBuffer(vma_allocator, buffer.buffer, buffer.allocation);
    });

    buffers.push_back(buffer);
    uint32_t id = buffers.size() - 1;
    buffer_id.push_back(name);
    return id;
}

uint32_t comp_allocator::create_img(VkFormat format, VkExtent3D extent,
                                    VkImageAspectFlags aspect,
                                    VkImageUsageFlags usage,
                                    VmaAllocationCreateFlags flags,
                                    std::string name)
{
    allocated_img img;

    VkImageCreateInfo img_info =
        vk_boiler::img_create_info(format, extent, usage);

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    img.format = format;

    VK_CHECK(vmaCreateImage(vma_allocator, &img_info, &vma_allocation_info,
                            &img.img, &img.allocation, nullptr));

    deletion_queue.push_back(
        [=]() { vmaDestroyImage(vma_allocator, img.img, img.allocation); });

    VkImageViewCreateInfo img_view_info =
        vk_boiler::img_view_create_info(aspect, img.img, extent, format);

    VK_CHECK(vkCreateImageView(device, &img_view_info, nullptr, &img.img_view));

    deletion_queue.push_back(
        [=]() { vkDestroyImageView(device, img.img_view, nullptr); });

    imgs.push_back(img);
    uint32_t id = imgs.size() - 1;
    img_id.push_back(name);
    return id;
}

VkDescriptorSetLayout comp_allocator::create_descriptor_set_layout(
    std::vector<VkDescriptorType>& types)
{
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;

    VkDescriptorSetLayoutCreateInfo layout_info =
        vk_boiler::descriptor_set_layout_create_info(
            types, VK_SHADER_STAGE_COMPUTE_BIT);

    VK_CHECK(
        vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout));

    deletion_queue.push_back(
        [=]() { vkDestroyDescriptorSetLayout(device, layout, nullptr); });

    return layout;
}

VkDescriptorSet comp_allocator::allocate_descriptor_set(
    VkDescriptorSetLayout layout)
{
    VkDescriptorSet set = VK_NULL_HANDLE;

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
        vk_boiler::descriptor_set_allocate_info(comp_descriptor_pool, &layout);

    VK_CHECK(
        vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &set));

    return set;
};

void cs::write_descriptor_set(std::vector<VkDescriptorType> types,
                              std::vector<std::string> names)
{
    for (uint32_t i = 0; i < types.size(); ++i) {
        VkDescriptorType type = types[i];
        std::string name = names[i];

        /* write descriptor sets with preset for each descriptor type */
        switch (type) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
            uint32_t buffer_id = allocator->get_buffer_id(name);
            VkDescriptorBufferInfo descriptor_buffer_info = {};
            descriptor_buffer_info.buffer =
                allocator->buffers[buffer_id].buffer;
            descriptor_buffer_info.offset = 0;
            descriptor_buffer_info.range =
                pad_uniform_buffer_size(allocator->buffers[buffer_id].size);

            VkWriteDescriptorSet write_set = vk_boiler::write_descriptor_set(
                &descriptor_buffer_info, set, i,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

            vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
        } break;

        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            uint32_t img_id = allocator->get_img_id(name);
            VkDescriptorImageInfo descriptor_img_info = {};
            descriptor_img_info.imageView = allocator->imgs[img_id].img_view;
            descriptor_img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet write_set = vk_boiler::write_descriptor_set(
                &descriptor_img_info, set, i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
        } break;

        default:
            break;
        }
    }
}

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

    VK_CHECK(
        vkCreateShaderModule(device, &shader_module_info, nullptr, &module));

    deletion_queue.push_back(
        [=]() { vkDestroyShaderModule(device, module, nullptr); });

    return true;
}

size_t cs::pad_uniform_buffer_size(size_t original_size)
{
    size_t aligned_size = original_size;
    if (min_buffer_alignment > 0)
        aligned_size = (aligned_size + min_buffer_alignment - 1) &
                       ~(min_buffer_alignment - 1);
    return aligned_size;
}
