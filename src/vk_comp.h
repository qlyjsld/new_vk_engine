#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "vk_boiler.h"
#include "vk_type.h"

struct comp_allocator {
public:
    comp_allocator(VkDevice device, VmaAllocator allocator)
        : device(device), allocator(allocator)
    {
        create_new_pool();
    };

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VmaAllocationCreateFlags flags, std::string name);

    void create_img(VkFormat format, VkExtent3D extent, VkImageAspectFlags aspect,
                    VkImageUsageFlags usage, VmaAllocationCreateFlags flags,
                    std::string name);

    inline allocated_buffer get_buffer(std::string name) { return buffers[name]; };
    inline allocated_img get_img(std::string name) { return imgs[name]; };

    void load_buffer(std::string name, allocated_buffer buffer)
    {
        buffers[name] = buffer;
    };

    void load_img(std::string name, allocated_img img) { imgs[name] = img; };

    void allocate_descriptor_set(std::vector<VkDescriptorType> types,
                                 VkDescriptorSetLayout *layout, VkDescriptorSet *set);

private:
    VkDevice device;
    VmaAllocator allocator;

    std::vector<VkDescriptorPool> pools;
    std::vector<VkDescriptorPool> full_pools;
    std::unordered_map<std::string, allocated_buffer> buffers;
    std::unordered_map<std::string, allocated_img> imgs;

    void create_new_pool();
    VkDescriptorPool get_pool() { return pools.back(); }
};

struct cs {
public:
    cs(comp_allocator allocator,
       std::vector<std::pair<VkDescriptorType, std::string>> descriptors,
       std::string file, VkDeviceSize min_uniform_buffer_offset_alignment,
       VkDevice device)
        : allocator(allocator),
          min_uniform_buffer_offset_alignment(min_uniform_buffer_offset_alignment),
          device(device)
    {
        std::vector<VkDescriptorType> types;
        std::vector<std::string> names;

        for (uint32_t i = 0; i < descriptors.size(); ++i) {
            types.push_back(descriptors[i].first);
            names.push_back(descriptors[i].second);
        }

        allocator.allocate_descriptor_set(types, &layout, &set);

        // for (uint32_t i = 0; i < descriptors.size(); ++i) {
        //     switch (types[i]) {
        //     case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        //         break;
        //     case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        //         break;
        //     case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        //         break;
        //     default:
        //         break;
        //     }
        // }

        VkDescriptorImageInfo descriptor_img_info = {};
        descriptor_img_info.imageView = allocator.get_img("target").img_view;
        descriptor_img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet write_set = vk_boiler::write_descriptor_set(
            &descriptor_img_info, set, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);

        VkDescriptorBufferInfo descriptor_buffer_info = {};
        descriptor_buffer_info.buffer = allocator.get_buffer("extent").buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = pad_uniform_buffer_size(sizeof(glm::vec2));

        write_set = vk_boiler::write_descriptor_set(
            &descriptor_buffer_info, set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

        vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);

        load_shader_module(file.data());
    };

    inline VkShaderModule get_module() { return module; };
    inline VkDescriptorSet get_set() { return set; };
    inline VkDescriptorSetLayout get_layout() { return layout; };
    inline VkPipeline get_pipeline() { return pipeline; };
    inline VkPipelineLayout get_pipeline_layout() { return pipeline_layout; };

    std::function<void()> draw;

private:
    VkDevice device;
    comp_allocator allocator;
    VkDeviceSize min_uniform_buffer_offset_alignment;

    VkShaderModule module;
    VkDescriptorSet set;
    VkDescriptorSetLayout layout;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    bool load_shader_module(const char *filename);
    size_t pad_uniform_buffer_size(size_t original_size);
};