#pragma once

#include "vk_camera.h"
#include "vk_mem_alloc.h"
#include "vk_mesh.h"
#include "vk_type.h"
#include <deque>
#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

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

struct mesh_push_constants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct frame {
    VkFence fence;
    VkSemaphore sumbit_sem, present_sem;
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffer;
};

class vk_engine
{
public:
    bool _is_initialized{false};
    uint64_t _frame_number{0};
    uint64_t _last_frame{0};
    VkExtent2D _window_extent{1600, 900};
    struct SDL_Window *_window{nullptr};

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_utils_messenger;
    VkPhysicalDevice _physical_device;
    VkDevice _device;
    VkSurfaceKHR _surface;

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_format;
    std::vector<VkImage> _swapchain_imgs;
    std::vector<VkImageView> _swapchain_img_views;
    uint32_t _img_index;

    VkQueue _gfx_queue;
    uint32_t _gfx_queue_family_index;
    frame _frames[FRAME_OVERLAP];
    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;

    VkShaderModule _vert;
    VkShaderModule _frag;

    VmaAllocator _allocator;
    std::vector<mesh> _meshes;

    VkImageView _depth_img_view;
    allocated_img _depth_img;
    VkFormat _depth_img_format;

    vk_camera _cam;
    deletion_queue _d_queue;

    void init();
    void cleanup();
    void draw();
    void run();

    frame *get_current_frame() { return &_frames[_frame_number % FRAME_OVERLAP]; };
    bool load_shader_module(const char *file, VkShaderModule *shader_module);

private:
    void device_init();
    void swapchain_init();
    void command_init();
    void sync_init();
    void camera_init() { _cam.init(); }
    void pipeline_init();

    void load_meshes();
    void upload_meshes(mesh *meshes, size_t size);
};
