﻿#pragma once

#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vk_camera.h"
#include "vk_mesh.h"
#include "vk_type.h"

constexpr int FRAME_OVERLAP = 2;

struct deletion_queue {
public:
    void push_back(std::function<void()> &&f) { fs.push_back(f); }

    void flush()
    {
        for (auto f = fs.rbegin(); f != fs.rend(); f++)
            (*f)();

        fs.clear();
    };

private:
    std::vector<std::function<void()>> fs;
};

struct frame {
    VkFence fence;
    VkSemaphore sumbit_sem, present_sem;
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffer;
};

struct upload_context {
    VkFence fence;
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffer;
};

struct mesh_push_constants {
    glm::vec4 data;
    glm::mat4 render_mat;
};

struct render_mat {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 model;
};

class vk_engine
{
public:
    bool _is_initialized{false};
    uint64_t _frame_number{0};
    uint64_t _last_frame{0};
    uint64_t _frame_index{0};
    VkExtent2D _window_extent{1600, 900};
    VkExtent2D _resolution{3840, 2160};
    struct SDL_Window *_window{nullptr};

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_utils_messenger;
    VkPhysicalDevice _physical_device;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkDeviceSize _minUniformBufferOffsetAlignment;

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_format;
    std::vector<VkImage> _swapchain_imgs;
    std::vector<VkImageView> _swapchain_img_views;
    uint32_t _img_index;

    frame _frames[FRAME_OVERLAP];

    VkDescriptorPool _descriptor_pool;
    VkDescriptorSetLayout _node_data_layout;
    VkDescriptorSet _node_data_set;
    allocated_buffer _node_data_buffer;
    VkDescriptorSetLayout _texture_layout;

    VkQueue _gfx_queue;
    uint32_t _gfx_queue_family_index;
    VkQueue _transfer_queue;
    uint32_t _transfer_queue_family_index;
    VkQueue _comp_queue;
    uint32_t _comp_queue_family_index;

    VmaAllocator _allocator;
    std::vector<mesh> _meshes;
    std::vector<node> _nodes;

    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;

    VkPipeline _comp_pipeline;
    VkPipelineLayout _comp_pipeline_layout;

    allocated_img _target;
    allocated_img _copy_to_swapchain;
    allocated_img _depth_img;

    vk_camera _cam;
    inline static deletion_queue _deletion_queue;

    upload_context _upload_context;
    void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&fs);

    void init();
    void cleanup();
    void draw();
    void run();

private:
    void device_init();
    void vma_init();
    void swapchain_init();
    void command_init();
    void sync_init();
    void descriptor_init();

    bool load_shader_module(const char *file, VkShaderModule *shader_module);
    void pipeline_init();

    void load_meshes();
    void upload_meshes(mesh *meshes, size_t size);
    void upload_textures(mesh *meshes, size_t size);

    void draw_nodes(frame *frame);

    frame *get_current_frame()
    {
        _frame_index = _frame_number % FRAME_OVERLAP;
        return &_frames[_frame_index];
    };

    allocated_buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VmaAllocationCreateFlags flags);

    allocated_img create_img(VkFormat format, VkExtent3D extent,
                             VkImageAspectFlags aspect, VkImageUsageFlags usage,
                             VmaAllocationCreateFlags flags);

    size_t pad_uniform_buffer_size(size_t original_size);
};
