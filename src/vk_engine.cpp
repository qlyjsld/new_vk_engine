﻿#include "vk_engine.h"

#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "vk_cmd.h"
#include "vk_init.h"
#include "vk_pipeline_builder.h"

void vk_engine::init()
{
    /* initialize SDL and create a window with it */
    SDL_Init(SDL_INIT_VIDEO);

    _window = SDL_CreateWindow("vk_engine", _window_extent.width, _window_extent.height,
                               SDL_WINDOW_VULKAN);

    _deletion_queue.push_back([=]() { SDL_DestroyWindow(_window); });

    device_init();

    VmaAllocatorCreateInfo vma_allocator_info = {};
    vma_allocator_info.physicalDevice = _physical_device;
    vma_allocator_info.device = _device;
    vma_allocator_info.instance = _instance;
    vmaCreateAllocator(&vma_allocator_info, &_allocator);

    _deletion_queue.push_back([=]() { vmaDestroyAllocator(_allocator); });

    swapchain_init();
    command_init();
    sync_init();

    descriptor_init();
    pipeline_init();

    load_meshes();
    std::cout << "meshes size " << _meshes.size() << std::endl;
    upload_meshes(_meshes.data(), _meshes.size());
    upload_textures(_meshes.data(), _meshes.size());

    _is_initialized = true;
}

void vk_engine::descriptor_init()
{
    std::vector<VkDescriptorPoolSize> desc_pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256}};

    VkDescriptorPoolCreateInfo desc_pool_info = vk_init::vk_create_descriptor_pool_info(
        desc_pool_sizes.size(), desc_pool_sizes.data());

    VK_CHECK(vkCreateDescriptorPool(_device, &desc_pool_info, nullptr, &_desc_pool));

    _deletion_queue.push_back(
        [=]() { vkDestroyDescriptorPool(_device, _desc_pool, nullptr); });

    /* node data layout and set */
    VkDescriptorSetLayoutBinding node_data_layout_binding_0 = {};
    node_data_layout_binding_0.binding = 0;
    node_data_layout_binding_0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    node_data_layout_binding_0.descriptorCount = 1;
    node_data_layout_binding_0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> node_data_layout_bindings = {
        node_data_layout_binding_0,
    };

    VkDescriptorSetLayoutCreateInfo node_data_layout_info =
        vk_init::vk_create_descriptor_set_layout_info(node_data_layout_bindings.size(),
                                                      node_data_layout_bindings.data());

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &node_data_layout_info, nullptr,
                                         &_node_data_layout));

    _deletion_queue.push_back(
        [=]() { vkDestroyDescriptorSetLayout(_device, _node_data_layout, nullptr); });

    VkDescriptorSetAllocateInfo desc_set_allocate_info =
        vk_init::vk_allocate_descriptor_set_info(_desc_pool, &_node_data_layout);

    VK_CHECK(vkAllocateDescriptorSets(_device, &desc_set_allocate_info, &_node_data_set));

    /* texture layout */
    VkDescriptorSetLayoutBinding texture_data_layout_binding_0 = {};
    texture_data_layout_binding_0.binding = 0;
    texture_data_layout_binding_0.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_data_layout_binding_0.descriptorCount = 1;
    texture_data_layout_binding_0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> texture_data_layout_bindings = {
        texture_data_layout_binding_0,
    };

    VkDescriptorSetLayoutCreateInfo texture_data_layout_info =
        vk_init::vk_create_descriptor_set_layout_info(
            texture_data_layout_bindings.size(), texture_data_layout_bindings.data());

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &texture_data_layout_info, nullptr,
                                         &_texture_layout));

    _deletion_queue.push_back(
        [=]() { vkDestroyDescriptorSetLayout(_device, _texture_layout, nullptr); });
}

void vk_engine::pipeline_init()
{
    PipelineBuilder gfx_pipeline_builder = {};

    if (!load_shader_module("../shaders/.vert.spv", &_vert))
        std::cerr << "load vert failed" << std::endl;
    else
        std::cout << "vert loaded" << std::endl;

    _deletion_queue.push_back([=]() { vkDestroyShaderModule(_device, _vert, nullptr); });

    gfx_pipeline_builder._shader_stage_infos.push_back(
        vk_init::vk_create_shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, _vert));

    if (!load_shader_module("../shaders/.frag.spv", &_frag))
        std::cerr << "load frag failed" << std::endl;
    else
        std::cout << "frag loaded" << std::endl;

    _deletion_queue.push_back([=]() { vkDestroyShaderModule(_device, _frag, nullptr); });

    gfx_pipeline_builder._shader_stage_infos.push_back(
        vk_init::vk_create_shader_stage_info(VK_SHADER_STAGE_FRAGMENT_BIT, _frag));

    gfx_pipeline_builder._vertex_input_state_info =
        vk_init::vk_create_vertex_input_state_info();
    gfx_pipeline_builder._input_asm_state_info =
        vk_init::vk_create_input_asm_state_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    gfx_pipeline_builder._rasterization_state_info =
        vk_init::vk_create_rasterization_state_info(VK_POLYGON_MODE_FILL);
    gfx_pipeline_builder._color_blend_attachment_state =
        vk_init::vk_create_color_blend_attachment_state();
    gfx_pipeline_builder._multisample_state_info =
        vk_init::vk_create_multisample_state_info();
    gfx_pipeline_builder._depth_stencil_state_info =
        vk_init::vk_create_depth_stencil_state_info();

    vertex_input_description description = vertex::get_vertex_input_description();
    gfx_pipeline_builder.customize(_window_extent, &description);

    std::vector<VkDescriptorSetLayout> layouts = {
        _node_data_layout,
        _texture_layout,
    };
    VkPipelineLayoutCreateInfo pipeline_layout_info =
        vk_init::vk_create_pipeline_layout_info(layouts);

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                    &gfx_pipeline_builder._pipeline_layout));

    _deletion_queue.push_back(
        [=]() { vkDestroyPipelineLayout(_device, _gfx_pipeline_layout, nullptr); });

    gfx_pipeline_builder.build_gfx(_device, &_swapchain_format, _depth_img.format);
    _gfx_pipeline = gfx_pipeline_builder.value();
    _gfx_pipeline_layout = gfx_pipeline_builder._pipeline_layout;

    _deletion_queue.push_back(
        [=]() { vkDestroyPipeline(_device, _gfx_pipeline, nullptr); });
}

void vk_engine::draw()
{
    /* block cpu accessing frame in used */
    frame *frame = get_current_frame();
    VK_CHECK(vkWaitForFences(_device, 1, &frame->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &frame->fence));

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
            _depth_img.img_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VkClearValue{1.f, 1.f, 1.f});

    /* start drawing */
    VkRenderingInfo rendering_info = vk_init::vk_create_rendering_info(
        &color_attachment, &depth_attachment, _window_extent);

    vkCmdBeginRendering(frame->cmd_buffer, &rendering_info);

    draw_nodes(frame);

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

    _last_frame = SDL_GetTicksNS();
    _frame_number++;
}

void vk_engine::draw_nodes(frame *frame)
{
    std::vector<node> nodes(_nodes);

    for (uint32_t i = 0; i < nodes.size(); ++i) {
        node *node = &nodes[i];

        for (auto c = node->children.cbegin(); c != node->children.cend(); ++c)
            nodes[*c].transform_mat = node->transform_mat * nodes[*c].transform_mat;

        if (node->mesh_id != -1) {
            mesh *mesh = &_meshes[node->mesh_id];
            vkCmdBindPipeline(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              _gfx_pipeline);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(frame->cmd_buffer, 0, 1, &mesh->vertex_buffer.buffer,
                                   &offset);

            vkCmdBindIndexBuffer(frame->cmd_buffer, mesh->index_buffer.buffer, 0,
                                 VK_INDEX_TYPE_UINT16);

            render_mat mat;
            mat.view = glm::lookAt(_cam.pos, _cam.pos + _cam.dir, _cam.up);
            mat.proj =
                glm::perspective(glm::radians(_cam.fov), 1600.f / 900.f, .01f, 65536.0f);
            mat.proj[1][1] *= -1;
            mat.model = node->transform_mat;

            void *data;
            vmaMapMemory(_allocator, _node_data_buffer.allocation, &data);
            std::memcpy((char *)data + i * pad_uniform_buffer_size(sizeof(render_mat)),
                        &mat, sizeof(render_mat));
            vmaUnmapMemory(_allocator, _node_data_buffer.allocation);

            std::vector<VkDescriptorSet> sets = {
                _node_data_set,
                mesh->desc_set,
            };
            uint32_t doffset = i * pad_uniform_buffer_size(sizeof(render_mat));
            vkCmdBindDescriptorSets(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    _gfx_pipeline_layout, 0, sets.size(), sets.data(), 1,
                                    &doffset);

            vkCmdDrawIndexed(frame->cmd_buffer, mesh->indices.size(), 1, 0, 0, 0);
        }
    }
}

void vk_engine::cleanup()
{
    vkDeviceWaitIdle(_device);

    if (_is_initialized)
        _deletion_queue.flush();
}

void vk_engine::run()
{
    SDL_Event e;
    bool bquit = false;

    uint32_t triangles = 0;
    for (uint32_t i = 0; i < _nodes.size(); ++i) {
        if (_nodes[i].mesh_id != -1)
            triangles += _meshes[_nodes[i].mesh_id].indices.size() / 3;
    }

    std::cout << "draw " << triangles << " triangels" << std::endl;

    while (!bquit) {
        const uint8_t *state = SDL_GetKeyboardState(NULL);

        if (state[SDL_SCANCODE_W]) {
            glm::vec3 v = _cam.dir * _cam.speed;
            _cam.move(v, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_A]) {
            glm::vec3 v = -_cam.right * _cam.speed;
            _cam.move(v, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_S]) {
            glm::vec3 v = -_cam.dir * _cam.speed;
            _cam.move(v, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_D]) {
            glm::vec3 v = _cam.right * _cam.speed;
            _cam.move(v, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_Q]) {
            float angle = _cam.sensitivity;
            _cam.rotate_yaw(angle, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_E]) {
            float angle = -_cam.sensitivity;
            _cam.rotate_yaw(angle, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_SPACE]) {
            glm::vec3 v = _cam.up * _cam.speed;
            _cam.move(v, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        if (state[SDL_SCANCODE_LCTRL]) {
            glm::vec3 v = -_cam.up * _cam.speed;
            _cam.move(v, (SDL_GetTicksNS() - _last_frame) / 1000.f);
        }

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT)
                bquit = true;

            if (e.type == SDL_EVENT_KEY_DOWN)
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    bquit = true;
        }

        draw();
    }
}
