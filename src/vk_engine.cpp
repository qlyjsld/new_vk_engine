#include "vk_engine.h"
#include "vk_mesh.h"
#include "vk_pipeline_builder.h"
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "vk_cmd.h"
#include "vk_init.h"
#include "vk_type.h"
#include "vk_util.h"

void vk_engine::init()
{
    /* initialize SDL and create a window with it */
    SDL_Init(SDL_INIT_VIDEO);

    _window = SDL_CreateWindow("vk_engine", _window_extent.width, _window_extent.height,
                               SDL_WINDOW_VULKAN);

    device_init();

    VmaAllocatorCreateInfo vma_allocator_info = {};
    vma_allocator_info.physicalDevice = _physical_device;
    vma_allocator_info.device = _device;
    vma_allocator_info.instance = _instance;
    vmaCreateAllocator(&vma_allocator_info, &_allocator);

    swapchain_init();
    command_init();
    sync_init();

    camera_init();
    pipeline_init();
    load_meshes();

    _is_initialized = true;
}

void vk_engine::device_init()
{
    /* create vulkan instance */
    vkb::InstanceBuilder vkb_instance_builder;
    vkb_instance_builder.set_app_name("vk_engine")
        .require_api_version(VKB_VK_API_VERSION_1_3)
        .request_validation_layers(true)
        .use_default_debug_messenger();

    auto vkb_instance_build_ret = vkb_instance_builder.build();

    if (!vkb_instance_build_ret) {
        std::cerr << "instance build failed: " << vkb_instance_build_ret.error().message()
                  << std::endl;
        abort();
    }

    vkb::Instance vkb_instance = vkb_instance_build_ret.value();
    _instance = vkb_instance.instance;
    _debug_utils_messenger = vkb_instance.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {};
    dynamic_rendering_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_features.pNext = nullptr;
    dynamic_rendering_features.dynamicRendering = VK_TRUE;

    /* select vulkan physical device, defaulted to discrete GPU */
    vkb::PhysicalDeviceSelector vkb_physical_device_selector{vkb_instance};
    vkb::PhysicalDevice vkb_physical_device =
        vkb_physical_device_selector
            .add_required_extension_features(dynamic_rendering_features)
            .set_surface(_surface)
            .select()
            .value();
    _physical_device = vkb_physical_device.physical_device;

    /* create vulkan device */
    vkb::DeviceBuilder vkb_device_builder{vkb_physical_device};
    vkb::Device vkb_device = vkb_device_builder.build().value();
    _device = vkb_device.device;

    /* get queue for commands */
    _gfx_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _gfx_queue_family_index =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void vk_engine::swapchain_init()
{
    vkb::SwapchainBuilder vkb_swapchain_builder{_physical_device, _device, _surface};
    vkb::Swapchain vkb_swapchain =
        vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(_window_extent.width, _window_extent.height)
            .build()
            .value();

    _swapchain = vkb_swapchain.swapchain;
    _swapchain_format = vkb_swapchain.image_format;
    _swapchain_imgs = vkb_swapchain.get_images().value();
    _swapchain_img_views = vkb_swapchain.get_image_views().value();

    _depth_img_format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo img_info = vk_init::vk_create_image_info(
        _depth_img_format, VkExtent3D{_window_extent.width, _window_extent.height, 1},
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &alloc_info, &_depth_img.img,
                            &_depth_img.allocation, nullptr));

    VkImageViewCreateInfo img_view_info = vk_init::vk_create_image_view_info(
        VK_IMAGE_ASPECT_DEPTH_BIT, _depth_img.img, _depth_img_format);

    VK_CHECK(vkCreateImageView(_device, &img_view_info, nullptr, &_depth_img_view));
}

void vk_engine::command_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkCommandPoolCreateInfo cmd_pool_info =
            vk_init::vk_create_cmd_pool_info(_gfx_queue_family_index);
        VK_CHECK(
            vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_frames[i].cmd_pool));

        VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
            vk_init::vk_create_cmd_buffer_allocate_info(1, _frames[i].cmd_pool);
        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                          &_frames[i].cmd_buffer));
    }
}

void vk_engine::sync_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkFenceCreateInfo fence_info = vk_init::vk_create_fence_info(true);
        VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_frames[i].fence));

        VkSemaphoreCreateInfo sem_info = vk_init::vk_create_sem_info();
        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr, &_frames[i].sumbit_sem));
        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr, &_frames[i].present_sem));
    }
}

void vk_engine::pipeline_init()
{
    PipelineBuilder graphics_pipeline_builder = {};

    if (!load_shader_module("../shaders/.vert.spv", &_vert))
        std::cerr << "load vert failed" << std::endl;
    else
        std::cout << "vert loaded" << std::endl;

    graphics_pipeline_builder._shader_stage_infos.push_back(
        vk_init::vk_create_shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, _vert));

    if (!load_shader_module("../shaders/.frag.spv", &_frag))
        std::cerr << "load frag failed" << std::endl;
    else
        std::cout << "frag loaded" << std::endl;

    graphics_pipeline_builder._shader_stage_infos.push_back(
        vk_init::vk_create_shader_stage_info(VK_SHADER_STAGE_FRAGMENT_BIT, _frag));
    graphics_pipeline_builder._vertex_input_state_info =
        vk_init::vk_create_vertex_input_state_info();
    graphics_pipeline_builder._input_asm_state_info =
        vk_init::vk_create_input_asm_state_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    graphics_pipeline_builder._rasterization_state_info =
        vk_init::vk_create_rasterization_state_info(VK_POLYGON_MODE_FILL);
    graphics_pipeline_builder._color_blend_attachment_state =
        vk_init::vk_create_color_blend_attachment_state();
    graphics_pipeline_builder._multisample_state_info =
        vk_init::vk_create_multisample_state_info();
    graphics_pipeline_builder._depth_stencil_state_info =
        vk_init::vk_create_depth_stencil_state_info();

    vertex_input_description description = vertex::get_vertex_input_description();
    graphics_pipeline_builder.customize(_window_extent, &description);

    VkPipelineLayoutCreateInfo pipeline_layout_info =
        vk_init::vk_create_pipeline_layout_info();

    VkPushConstantRange push_constant_range = {};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(mesh_push_constants);

    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                    &graphics_pipeline_builder._pipeline_layout));

    graphics_pipeline_builder.build(_device, &_swapchain_format, _depth_img_format);
    _gfx_pipeline = graphics_pipeline_builder.value();
    _gfx_pipeline_layout = graphics_pipeline_builder._pipeline_layout;
}

void vk_engine::load_meshes()
{
    mesh duck("../assets/glTF-Sample-Assets/Models/Duck/glTF-Binary/Duck.glb");
    upload_meshes(&duck, 1);
}

void vk_engine::upload_meshes(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh mesh = meshes[i];

        /* create vertex buffer */
        VkBufferCreateInfo vertex_buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        vertex_buffer_info.size = mesh.vertices.size() * sizeof(vertex);
        vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo vma_allocation_info = {};
        vma_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        vmaCreateBuffer(_allocator, &vertex_buffer_info, &vma_allocation_info,
                        &mesh.vertex_buffer.buffer, &mesh.vertex_buffer.allocation,
                        nullptr);

        void *data;
        vmaMapMemory(_allocator, mesh.vertex_buffer.allocation, &data);
        std::memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(vertex));
        vmaUnmapMemory(_allocator, mesh.vertex_buffer.allocation);

        /* create index buffer */
        VkBufferCreateInfo index_buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        index_buffer_info.size = mesh.indices.size() * sizeof(uint16_t);
        index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo vma_allocation_info_2 = {};
        vma_allocation_info_2.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        vmaCreateBuffer(_allocator, &index_buffer_info, &vma_allocation_info_2,
                        &mesh.index_buffer.buffer, &mesh.index_buffer.allocation,
                        nullptr);

        vmaMapMemory(_allocator, mesh.index_buffer.allocation, &data);
        std::memcpy(data, mesh.indices.data(), mesh.indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(_allocator, mesh.index_buffer.allocation);

        _meshes.push_back(mesh);
    }
}

void vk_engine::draw()
{
    /* block cpu accessing frame in used */
    frame *frame = get_current_frame();
    VK_CHECK(vkWaitForFences(_device, 1, &frame->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &frame->fence));
    VK_CHECK(vkResetCommandBuffer(frame->cmd_buffer, 0x00000000));

    /* wait and acquire the next frame */
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, frame->present_sem,
                                   VK_NULL_HANDLE, &_img_index));

    /* prepare command buffer and dynamic rendering functions */
    VkCommandBufferBeginInfo cmd_buffer_begin_info =
        vk_init::vk_create_cmd_buffer_begin_info();

    /* begin command buffer recording */
    VK_CHECK(vkBeginCommandBuffer(frame->cmd_buffer, &cmd_buffer_begin_info));

    /* transition image format for rendering */
    vk_cmd::vk_img_layout_transition(
        frame->cmd_buffer, _swapchain_imgs[_img_index], VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _gfx_queue_family_index);

    /* frame attachment info */
    VkRenderingAttachmentInfo color_attachment =
        vk_init::vk_create_rendering_attachment_info(
            _swapchain_img_views[_img_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VkClearValue{1.f, 1.f, 1.f});

    VkRenderingAttachmentInfo depth_attachment =
        vk_init::vk_create_rendering_attachment_info(
            _depth_img_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VkClearValue{1.f, 1.f, 1.f});

    /* start drawing */
    VkRenderingInfo rendering_info = vk_init::vk_create_rendering_info(
        &color_attachment, &depth_attachment, _window_extent);

    vkCmdBeginRendering(frame->cmd_buffer, &rendering_info);

    for (uint32_t i = 0; i < _meshes.size(); ++i) {
        mesh *mesh = &_meshes[i];

        vkCmdBindPipeline(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          _gfx_pipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(frame->cmd_buffer, 0, 1, &mesh->vertex_buffer.buffer,
                               &offset);

        vkCmdBindIndexBuffer(frame->cmd_buffer, mesh->index_buffer.buffer, 0,
                             VK_INDEX_TYPE_UINT16);

        glm::mat4 view = glm::translate(glm::mat4(1.f), -glm::vec3(_cam.pos));

        glm::mat4 projection =
            glm::perspective(glm::radians(68.f), 1600.f / 900.f, 0.1f, 1024.0f);

        projection[1][1] *= -1;

        glm::mat4 model =
            glm::translate(glm::mat4(1.f), glm::vec3(0.f, -64.f, -128.f)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(_frame_number * 0.01f),
                        glm::vec3(0.f, 1.f, 0.f));

        mesh_push_constants push_constants;
        push_constants.render_matrix = projection * view * model;

        vkCmdPushConstants(frame->cmd_buffer, _gfx_pipeline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mesh_push_constants),
                           &push_constants);

        vkCmdDrawIndexed(frame->cmd_buffer, mesh->indices.size(), 1, 0, 0, 0);
    }

    vkCmdEndRendering(frame->cmd_buffer);

    /* transition image format for presenting */
    vk_cmd::vk_img_layout_transition(frame->cmd_buffer, _swapchain_imgs[_img_index],
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                     _gfx_queue_family_index);

    VK_CHECK(vkEndCommandBuffer(frame->cmd_buffer));

    /* submit and present queue */
    VkPipelineStageFlags pipeline_stage_flags = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

    VkSubmitInfo submit_info =
        vk_init::vk_create_submit_info(&frame->cmd_buffer, &frame->present_sem,
                                       &frame->sumbit_sem, &pipeline_stage_flags);
    VK_CHECK(vkQueueSubmit(_gfx_queue, 1, &submit_info, frame->fence));

    VkPresentInfoKHR present_info =
        vk_init::vk_create_present_info(&_swapchain, &frame->sumbit_sem, &_img_index);
    VK_CHECK(vkQueuePresentKHR(_gfx_queue, &present_info));

    _last_frame = SDL_GetTicks();
    _frame_number++;
}

void vk_engine::cleanup()
{
    vkDeviceWaitIdle(_device);

    if (_is_initialized) {
        for (uint32_t i = 0; i < _meshes.size(); i++) {
            vmaDestroyBuffer(_allocator, _meshes[i].vertex_buffer.buffer,
                             _meshes[i].vertex_buffer.allocation);
            vmaDestroyBuffer(_allocator, _meshes[i].index_buffer.buffer,
                             _meshes[i].index_buffer.allocation);
        }

        vkDestroyPipeline(_device, _gfx_pipeline, nullptr);
        vkDestroyPipelineLayout(_device, _gfx_pipeline_layout, nullptr);
        vkDestroyShaderModule(_device, _frag, nullptr);
        vkDestroyShaderModule(_device, _vert, nullptr);

        for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
            vkDestroySemaphore(_device, _frames[i].present_sem, nullptr);
            vkDestroySemaphore(_device, _frames[i].sumbit_sem, nullptr);
            vkDestroyFence(_device, _frames[i].fence, nullptr);
            vkDestroyCommandPool(_device, _frames[i].cmd_pool, nullptr);
        }

        vkDestroyImageView(_device, _depth_img_view, nullptr);
        vmaDestroyImage(_allocator, _depth_img.img, _depth_img.allocation);

        for (uint32_t i = 0; i < _swapchain_img_views.size(); i++)
            vkDestroyImageView(_device, _swapchain_img_views[i], nullptr);

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        vmaDestroyAllocator(_allocator);
        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_utils_messenger, nullptr);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }
}

void vk_engine::run()
{
    SDL_Event e;
    bool bquit = false;

    while (!bquit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT)
                bquit = true;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    bquit = true;
                std::cout << SDL_GetKeyName(e.key.keysym.sym) << " pressed " << std::endl;
            }
        }

        draw();
    }
}

bool vk_engine::load_shader_module(const char *filename, VkShaderModule *shader_module)
{
    std::ifstream f(filename, std::ios::ate | std::ios::binary);

    if (!f.is_open()) {
        std::cerr << "shader: " << filename << " not exist" << std::endl;
        return false;
    }

    size_t size = f.tellg();
    std::vector<uint32_t> buffer(size / sizeof(uint32_t));

    f.seekg(0);
    f.read((char *)buffer.data(), size);
    f.close();

    VkShaderModuleCreateInfo shader_module_info = {};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.pNext = nullptr;
    shader_module_info.codeSize = buffer.size() * sizeof(uint32_t);
    shader_module_info.pCode = buffer.data();

    VK_CHECK(vkCreateShaderModule(_device, &shader_module_info, nullptr, shader_module));

    return true;
}
