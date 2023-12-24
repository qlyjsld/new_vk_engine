#pragma once

#include <deque>
#include <functional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vk_camera.h"
#include "vk_mem_alloc.h"
#include "vk_mesh.h"
#include "vk_type.h"

constexpr int FRAME_OVERLAP = 2;

struct deletion_queue {
public:
    std::deque<std::function<void()>> fs;

    void push_back(std::function<void()> &&f) { fs.push_back(f); }

    void flush()
    {
        for (auto f = fs.rbegin(); f != fs.rend(); f++)
            (*f)();

        fs.clear();
    };
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

class vk_engine
{
public:
    bool _is_initialized{false};
    uint64_t _frame_number{0};
    uint64_t _last_frame{0};
    uint64_t _frame_index{0};
    VkExtent2D _window_extent{1600, 900};
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

    VkQueue _gfx_queue;
    uint32_t _gfx_queue_family_index;
    frame _frames[FRAME_OVERLAP];

    VkShaderModule _vert;
    VkShaderModule _frag;

    VmaAllocator _allocator;
    std::vector<mesh> _meshes;
    std::vector<glm::mat4> _transform_mat;
    std::vector<material> _materials;

    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;
    VkDescriptorPool _desc_pool;
    VkDescriptorSetLayout _desc_set_layout;
    VkDescriptorSet _desc_set;
    allocated_buffer _mat_buffer;

    allocated_img _depth_img;

    VkQueue _transfer_queue;
    uint32_t _transfer_queue_family_index;
    upload_context _upload_context;
    void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&fs);

    vk_camera _cam;
    deletion_queue _deletion_queue;

    void init();
    void cleanup();
    void draw();
    void run();

private:
    void device_init();
    void swapchain_init();
    void command_init();
    void sync_init();
    void camera_init() { _cam.init(); }
    void descriptor_init();
    void pipeline_init();

    bool load_shader_module(const char *file, VkShaderModule *shader_module);

    void load_meshes();
    void upload_meshes(mesh *meshes, size_t size);
    void upload_textures(mesh *meshes, size_t size);
    void draw_meshes(frame *frame);

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
