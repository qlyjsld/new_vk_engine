#pragma once

#include <vector>
#include <volk.h>

#include "vk_mem_alloc.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vk_camera.h"
#include "vk_mesh.h"
#include "vk_type.h"

constexpr int FRAME_OVERLAP = 2;

struct frame {
    VkFence fence;
    VkSemaphore sumbit_sem, present_sem;
    VkCommandPool cpool;
    VkCommandBuffer cbuffer;
};

struct upload_context {
    VkFence fence;
    VkCommandPool cpool;
    VkCommandBuffer cbuffer;
};

struct push_constants {
    glm::vec4 dummy;
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
    bool _is_initialized = false;
    uint64_t _frame_number = 0;
    uint64_t _last_frame = 0;
    uint64_t _frame_index = 0;
    VkExtent2D _window_extent = { 1024, 768 };
    static constexpr VkExtent2D _resolution = { 1024, 768 };
    struct SDL_Window *_window = nullptr;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_utils_messenger;
    VkPhysicalDevice _physical_device;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkDeviceSize _min_buffer_alignment;

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_format;
    std::vector<VkImage> _swapchain_imgs;
    std::vector<VkImageView> _swapchain_img_views;
    uint32_t _img_index;

    frame _frames[FRAME_OVERLAP];
    VkSampler _sampler;

    VkDescriptorPool _descriptor_pool;
    VkDescriptorSetLayout _render_mat_layout;
    VkDescriptorSet _render_mat_set;
    allocated_buffer _render_mat_buffer;
    VkDescriptorSetLayout _texture_layout;

    VkQueue _gfx_queue;
    uint32_t _gfx_index;
    VkQueue _transfer_queue;
    uint32_t _transfer_index;
    VkQueue _comp_queue;
    uint32_t _comp_index;

    VmaAllocator _allocator;
    std::vector<mesh> _meshes;
    std::vector<node> _nodes;

    VkShaderModule _vert;
    VkShaderModule _frag;

    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;

    VkFormat _format = { VK_FORMAT_R16G16B16A16_SFLOAT };
    VkColorSpaceKHR _colorspace = { VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    allocated_img _target;
    allocated_img _depth_img;

    vk_camera _vk_camera;

    upload_context _upload_context;
    void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&fs);

    void init();
    void cleanup();
    void draw();
    void run();

private:
    VmaVulkanFunctions vma_vulkan_func;

    void device_init();
    void vma_init();
    void swapchain_init();
    void command_init();
    void sync_init();

    void descriptor_init();
    bool load_shader_module(const char *file, VkShaderModule *shader_module);
    void pipeline_init();

    void imgui_init();

    void load_meshes();
    void upload_meshes(mesh *meshes, size_t size);
    void upload_textures(mesh *meshes, size_t size);

    void comp_init();
    void cloudtex_init();
    void weather_init();
    void cloud_init();

    void draw_comp(frame *frame);
    void draw_nodes(frame *frame);

    frame *get_current_frame()
    {
        _frame_index = _frame_number % FRAME_OVERLAP;
        return &_frames[_frame_index];
    };

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VmaAllocationCreateFlags flags, allocated_buffer *buffer);

    void create_img(VkFormat format, VkExtent3D extent, VkImageAspectFlags aspect,
                    VkImageUsageFlags usage, VmaAllocationCreateFlags flags,
                    allocated_img *img);

    size_t pad_uniform_buffer_size(size_t original_size);
};
