#pragma once

#include "vk_type.h"
#include <vector>
#include <vulkan/vulkan_core.h>

class vk_engine
{
public:
    bool _is_initialized{false};
    int _frame_number{0};
    VkExtent2D _window_extent{800, 600};
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
    VkQueue _gfx_queue;
    uint32_t _gfx_queue_family_index;
    VkCommandPool _cmd_pool;
    VkCommandBuffer _cmd_buffer;
    VkFence _fence;
    VkSemaphore _sumbit_sem, _present_sem;
    uint32_t _img_index;
    VkPipeline _gfx_pipeline;
    VkPipelineLayout _gfx_pipeline_layout;

    VkShaderModule _vert;
    VkShaderModule _frag;

    void init();
    void cleanup();
    void draw();
    void run();

    bool load_shader_module(const char *file, VkShaderModule *shader_module);

private:
    void device_init();
    void swapchain_init();
    void command_init();
    void sync_init();
    void pipeline_init();
};

class PipelineBuilder
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stage_infos;
    VkPipelineVertexInputStateCreateInfo _vertex_input_state_info;
    VkPipelineInputAssemblyStateCreateInfo _input_asm_state_info;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterization_state_info;
    VkPipelineColorBlendAttachmentState _color_blend_attachment_state;
    VkPipelineMultisampleStateCreateInfo _multisample_state_info;
    VkPipelineLayout _pipeline_layout;
    VkPipeline _pipeline;

    void build(VkDevice device, VkFormat *format);
    VkPipeline value() { return _pipeline; };
};
