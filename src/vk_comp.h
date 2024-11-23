#pragma once

#include <functional>
#include <utility>
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
    VkDevice device;
    VmaAllocator allocator;

    comp_allocator(VkDevice device, VmaAllocator allocator)
        : device(device), allocator(allocator){};

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
    inline static std::vector<VkDescriptorPool> pools;
    inline static std::vector<VkDescriptorPool> full_pools;
    inline static std::unordered_map<std::string, allocated_buffer> buffers;
    inline static std::unordered_map<std::string, allocated_img> imgs;

    void create_new_pool();
    VkDescriptorPool get_pool();
};

struct cs {
public:
    cs(comp_allocator allocator, std::vector<descriptor> descriptors,
       std::string shader_file, VkDeviceSize min_buffer_alignment)
        : allocator(allocator), min_buffer_alignment(min_buffer_alignment)
    {
        device = allocator.device;

        std::vector<VkDescriptorType> types;
        std::vector<std::string> names;

        for (uint32_t i = 0; i < descriptors.size(); ++i) {
            types.push_back(descriptors[i].first);
            names.push_back(descriptors[i].second);
        }

        allocator.allocate_descriptor_set(types, &layout, &set);

        write_descriptor_set(types, names);

        load_shader_module(shader_file.data());
    };

    std::function<void(VkCommandBuffer, cs *cs)> draw;

    comp_allocator allocator;

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

inline static std::vector<cs> css;
