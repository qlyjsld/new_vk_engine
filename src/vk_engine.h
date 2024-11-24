#pragma once

#include <vector>
#include <volk.h>

#include "vk_mem_alloc.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vk_camera.h"
#include "vk_mesh.h"
#include "vk_type.h"
#include "vk_comp.h"

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

struct camera_data {
    glm::vec3 pos;
    float fov;
    glm::vec3 dir;
    float width;
    glm::vec3 up;
    float height;
};

struct cloud_data {
    float type;
    float freq;
    float ambient;
    float sigma_a;
    float sigma_s;
    float step;
    int max_steps;
    float cutoff;
    glm::vec3 sun_color;
    float density;
    glm::vec3 sky_color;
};

class vk_engine
{
public:
    // data used every frame
    vk_camera _vk_camera;
    float u_time = 0.f;
    uint32_t _frame_index = 0;

    cloud_data _cloud_data;
    uint32_t _last_frame = 0;

    camera_data _camera_data;
    struct SDL_Window *_window = nullptr;
    VkDevice _device;

    frame _frames[FRAME_OVERLAP];

    std::vector<std::function<void(VkCommandBuffer)>> cs_draw;
    std::vector<VkImage> _swapchain_imgs;

    VkSwapchainKHR _swapchain;
    uint32_t _img_index;
    uint32_t _gfx_index;
    uint32_t _transfer_index;
    uint32_t _comp_index;
    allocated_img _target;

    allocated_img _depth_img;
    VkQueue _gfx_queue;
    VkQueue _transfer_queue;
    VkQueue _comp_queue;

    comp_allocator _comp_allocator;
    bool cloud_ui = true;

    VkExtent2D _window_extent = { 1024, 768 };
    static constexpr VkExtent2D _resolution = { 1024, 768 };

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_utils_messenger;
    VkPhysicalDevice _physical_device;
    VkSurfaceKHR _surface;
    VkFormat _swapchain_format;
    VkFormat _format = { VK_FORMAT_B8G8R8A8_UNORM };
    std::vector<VkImageView> _swapchain_img_views;

    VkSampler _sampler;
    VkDeviceSize _min_buffer_alignment;

    VkDescriptorPool _descriptor_pool;
    VkDescriptorSetLayout _render_mat_layout;
    VkDescriptorSet _render_mat_set;
    allocated_buffer _render_mat_buffer;
    VkDescriptorSetLayout _texture_layout;

    VmaAllocator _allocator;
    std::vector<mesh> _meshes;
    std::vector<node> _nodes;

    VkShaderModule _vert;
    VkShaderModule _frag;

    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;

    VkColorSpaceKHR _colorspace = { VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

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

    inline frame *get_current_frame()
    {
        _frame_index = !_frame_index;
        return &_frames[_frame_index];
    };

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VmaAllocationCreateFlags flags, allocated_buffer *buffer);

    void create_img(VkFormat format, VkExtent3D extent, VkImageAspectFlags aspect,
                    VkImageUsageFlags usage, VmaAllocationCreateFlags flags,
                    allocated_img *img);

    size_t pad_uniform_buffer_size(size_t original_size);
};
