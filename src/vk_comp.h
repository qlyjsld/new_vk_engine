#pragma once

#include <functional>
#include <string>
#include <vector>
#include <volk.h>

#include "vk_mem_alloc.h"

#include "vk_type.h"

typedef std::pair<VkDescriptorType, std::string> descriptor;

struct comp_context {
    VkFence fence;
    VkCommandPool cpool;
    VkCommandBuffer cbuffer;
};

struct comp_allocator {
public:
    std::vector<allocated_buffer> buffers;
    std::vector<allocated_img> imgs;

    VkDevice device;
    VmaAllocator vma_allocator;

    uint32_t create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VmaAllocationCreateFlags flags, std::string name);

    uint32_t create_img(VkFormat format, VkExtent3D extent,
                        VkImageAspectFlags aspect, VkImageUsageFlags usage,
                        VmaAllocationCreateFlags flags, std::string name);

    inline uint32_t get_buffer_id(std::string name)
    {
        uint32_t i = 0;
        for (i = 0; i < buffer_id.size(); ++i)
            if (buffer_id[i] == name)
                return i;
        return i;
    };

    inline uint32_t get_img_id(std::string name)
    {
        uint32_t i = 0;
        for (i = 0; i < img_id.size(); ++i)
            if (img_id[i] == name)
                return i;
        return i;
    };

    inline void load_buffer(std::string name, allocated_buffer buffer)
    {
        buffers.push_back(buffer);
        buffer_id.push_back(name);
    };

    inline void load_img(std::string name, allocated_img img)
    {
        imgs.push_back(img);
        img_id.push_back(name);
    }

    void allocate_descriptor_set(std::vector<VkDescriptorType> types,
                                 VkDescriptorSetLayout *layout,
                                 VkDescriptorSet *set);

private:
    std::vector<VkDescriptorPool> pools;
    std::vector<VkDescriptorPool> full_pools;
    std::vector<std::string> buffer_id;
    std::vector<std::string> img_id;

    void create_new_pool();
    VkDescriptorPool get_pool();
};

struct cs {
public:
    cs(comp_allocator *allocator, std::vector<descriptor> descriptors,
       std::string shader_file, VkDeviceSize min_buffer_alignment)
        : allocator(allocator), min_buffer_alignment(min_buffer_alignment)
    {
        device = allocator->device;

        std::vector<VkDescriptorType> types;
        std::vector<std::string> names;

        for (uint32_t i = 0; i < descriptors.size(); ++i) {
            types.push_back(descriptors[i].first);
            names.push_back(descriptors[i].second);
        }

        allocator->allocate_descriptor_set(types, &layout, &set);

        write_descriptor_set(types, names);

        load_shader_module(shader_file.data());
    };

    std::function<void(VkCommandBuffer)> immed_draw;

    comp_allocator *allocator;

    VkShaderModule module;
    VkDescriptorSet set;
    VkDescriptorSetLayout layout;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    static void cc_init(uint32_t queue_index, VkDevice device);
    static void comp_immediate_submit(VkDevice device, VkQueue queue, cs *cs);

private:
    VkDeviceSize min_buffer_alignment;
    VkDevice device;

    inline static comp_context cc;

    void write_descriptor_set(std::vector<VkDescriptorType> types,
                              std::vector<std::string> names);

    bool load_shader_module(const char *filename);
    size_t pad_uniform_buffer_size(size_t original_size);
};
