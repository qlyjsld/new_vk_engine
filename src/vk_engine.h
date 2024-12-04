#pragma once

#include <vector>
#include <volk.h>

#include "vk_mem_alloc.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "vk_camera.h"
#include "vk_comp.h"
#include "vk_mesh.h"
#include "vk_type.h"

constexpr int FRAME_OVERLAP = 2;

struct frame {
    VkFence fence;
    VkSemaphore sumbit_sem, present_sem;
    VkCommandPool cpool;
    VkCommandBuffer cbuffer;
};

struct immed_context {
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
    glm::vec3 left;
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

    camera_data _camera_data;
    VkDevice _device;
    struct SDL_Window *_window = nullptr;

    frame _frames[FRAME_OVERLAP];

    std::vector<std::function<void(VkCommandBuffer)>> cs_draw;
    std::vector<VkImage> _swapchain_imgs;

    VkSwapchainKHR _swapchain;
    allocated_img _target;
    uint64_t _last_frame = 0;
    uint32_t _img_index;
    uint32_t _fam_index = 0;

    VkFormat _format = {VK_FORMAT_B8G8R8A8_UNORM};
    VkColorSpaceKHR _colorspace = {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    allocated_img _depth_img;
    VkQueue _queue;
    VmaAllocator _allocator;

    bool cloud_ui = true;
    comp_allocator _comp_allocator;
    VkExtent2D _window_extent = {1024, 768};
    VkExtent2D _resolution = {1024, 768};

    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_utils_messenger;
    VkPhysicalDevice _physical_device;
    VkSurfaceKHR _surface;
    std::vector<VkImageView> _swapchain_img_views;

    VkSampler _sampler;
    VkDeviceSize _min_buffer_alignment;

    VkDescriptorPool _descriptor_pool;
    VkDescriptorSetLayout _render_mat_layout;
    VkDescriptorSet _render_mat_set;
    allocated_buffer _render_mat_buffer;
    VkDescriptorSetLayout _texture_layout;

    std::vector<mesh> _meshes;
    std::vector<node> _nodes;

    VkShaderModule _vert;
    VkShaderModule _frag;

    immed_context _immed_context;

    void immediate_draw(std::function<void(VkCommandBuffer cmd)> &&fs,
                        VkQueue queue);

    void init();
    void cleanup();
    void draw();
    void run();

private:
    VmaVulkanFunctions vma_vulkan_func;

    constexpr static VkClearValue clear_value = {{{1.f}}};

    void device_init();
    void vma_init();
    void swapchain_init();
    void command_init();
    void sync_init();

    void descriptor_init();
    VkShaderModule load_shader_module(const char *file);
    void pipeline_init();

    void imgui_init();

    void load_meshes();
    void upload_meshes(mesh *meshes, size_t size);
    void upload_textures(mesh *meshes, size_t size);

    void comp_init();
    void cloudtex_init();
    void weather_init();
    void cloud_init();

    void draw_imgui();
    void draw_comp(frame *frame);
    void draw_nodes(frame *frame);

    inline frame *get_current_frame()
    {
        _frame_index = !_frame_index;
        return &_frames[_frame_index];
    };

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VmaAllocationCreateFlags flags,
                       allocated_buffer *buffer);

    void create_img(VkFormat format, VkExtent3D extent,
                    VkImageAspectFlags aspect, VkImageUsageFlags usage,
                    VmaAllocationCreateFlags flags, allocated_img *img);

    size_t pad_uniform_buffer_size(size_t original_size);
};
