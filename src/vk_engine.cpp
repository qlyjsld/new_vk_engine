#include "vk_engine.h"

#include <future>
#include <iostream>
#include <vector>
#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VK_NO_PROTOTYPES

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_pipeline.h"
#include "vk_type.h"

void vk_engine::init()
{
    /* initialize SDL and create a window with it */
    SDL_Init(SDL_INIT_VIDEO);

    _window = SDL_CreateWindow("vk_engine", _window_extent.width, _window_extent.height,
                               SDL_WINDOW_VULKAN);

    deletion_queue.push_back([=]() { SDL_DestroyWindow(_window); });

    device_init();

    volkInitialize();
    volkLoadInstance(_instance);
    volkLoadDevice(_device);

    vma_vulkan_func = {};
    vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties =
        vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
    vma_vulkan_func.vkFreeMemory = vkFreeMemory;
    vma_vulkan_func.vkMapMemory = vkMapMemory;
    vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
    vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
    vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
    vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
    vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
    vma_vulkan_func.vkCreateImage = vkCreateImage;
    vma_vulkan_func.vkDestroyImage = vkDestroyImage;
    vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    vma_vulkan_func.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    vma_vulkan_func.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
    vma_vulkan_func.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR =
        vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
#if VMA_KHR_MAINTENANCE4 || VMA_VULKAN_VERSION >= 1003000
    vma_vulkan_func.vkGetDeviceBufferMemoryRequirements =
        vkGetDeviceBufferMemoryRequirements;
    vma_vulkan_func.vkGetDeviceImageMemoryRequirements =
        vkGetDeviceImageMemoryRequirements;
#endif

    vma_init();

    swapchain_init();
    command_init();
    sync_init();

    descriptor_init();
    pipeline_init();

    imgui_init();

    // load_meshes();
    // std::cout << "meshes size " << _meshes.size() << std::endl;
    // upload_meshes(_meshes.data(), _meshes.size());
    // upload_textures(_meshes.data(), _meshes.size());

    comp_init();

    _is_initialized = true;
}

void vk_engine::descriptor_init()
{
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256},
    };

    VkDescriptorPoolCreateInfo pool_info =
        vk_boiler::descriptor_pool_create_info(pool_sizes.size(), pool_sizes.data());

    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pool));

    deletion_queue.push_back(
        [=]() { vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr); });

    { /* render mat layout and set */
        VkDescriptorSetLayoutCreateInfo render_mat_layout_info =
            vk_boiler::descriptor_set_layout_create_info(
                std::vector<VkDescriptorType>{
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                },
                VK_SHADER_STAGE_VERTEX_BIT);

        VK_CHECK(vkCreateDescriptorSetLayout(_device, &render_mat_layout_info, nullptr,
                                             &_render_mat_layout));

        deletion_queue.push_back([=]() {
            vkDestroyDescriptorSetLayout(_device, _render_mat_layout, nullptr);
        });

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
            vk_boiler::descriptor_set_allocate_info(_descriptor_pool,
                                                    &_render_mat_layout);

        VK_CHECK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info,
                                          &_render_mat_set));
    }

    { /* texture layout */
        VkDescriptorSetLayoutCreateInfo texture_data_layout_info =
            vk_boiler::descriptor_set_layout_create_info(
                std::vector<VkDescriptorType>{
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                },
                VK_SHADER_STAGE_FRAGMENT_BIT);

        VK_CHECK(vkCreateDescriptorSetLayout(_device, &texture_data_layout_info, nullptr,
                                             &_texture_layout));

        deletion_queue.push_back(
            [=]() { vkDestroyDescriptorSetLayout(_device, _texture_layout, nullptr); });
    }
}

void vk_engine::pipeline_init()
{
    /* build graphics pipeline */
    load_shader_module("../shaders/.vert.spv", &_vert);
    load_shader_module("../shaders/.frag.spv", &_frag);

    PipelineBuilder gfx_pipeline_builder = {};
    gfx_pipeline_builder._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, _vert));
    gfx_pipeline_builder._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, _frag));

    gfx_pipeline_builder._viewport = vk_boiler::viewport(_resolution);
    gfx_pipeline_builder._scissor = vk_boiler::scissor(_resolution);

    vertex_input_description description = vertex::get_vertex_input_description();

    gfx_pipeline_builder._vertex_input_state_info =
        vk_boiler::vertex_input_state_create_info(&description);
    gfx_pipeline_builder._input_asm_state_info =
        vk_boiler::input_asm_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    gfx_pipeline_builder._rasterization_state_info =
        vk_boiler::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    gfx_pipeline_builder._color_blend_attachment_state =
        vk_boiler::color_blend_attachment_state();
    gfx_pipeline_builder._multisample_state_info =
        vk_boiler::multisample_state_create_info();
    gfx_pipeline_builder._depth_stencil_state_info =
        vk_boiler::depth_stencil_state_create_info();

    std::vector<VkDescriptorSetLayout> layouts = {
        _render_mat_layout,
        _texture_layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    gfx_pipeline_builder.build_layout(_device, layouts, push_constants,
                                        &_gfx_pipeline_layout);

    gfx_pipeline_builder.build_gfx(_device, &_format, _depth_img.format,
                                    &_gfx_pipeline_layout, &_gfx_pipeline);
}

void vk_engine::draw()
{
    /* block cpu accessing frame in used */
    frame *frame = get_current_frame();
    VK_CHECK(vkWaitForFences(_device, 1, &frame->fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &frame->fence));

    /* wait and acquire the next frame */
    vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, frame->present_sem,
                          VK_NULL_HANDLE, &_img_index);

    /* prepare command buffer and dynamic rendering functions */
    VkCommandBufferBeginInfo cbuffer_begin_info = vk_boiler::cbuffer_begin_info();

    /* begin command buffer recording */
    VK_CHECK(vkBeginCommandBuffer(frame->cbuffer, &cbuffer_begin_info));

    /* transition image format for rendering */
    vk_cmd::vk_img_layout_transition(
        frame->cbuffer, _target.img, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _gfx_index);

    /* draw with comp */
    draw_comp(frame);

    /* frame attachment info */
    VkRenderingAttachmentInfo color_attachment = vk_boiler::rendering_attachment_info(
        _target.img_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false,
        VkClearValue{1.f});

    VkRenderingAttachmentInfo depth_attachment = vk_boiler::rendering_attachment_info(
        _depth_img.img_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, true,
        VkClearValue{1.f});

    /* start drawing */
    VkRenderingInfo rendering_info =
        vk_boiler::rendering_info(&color_attachment, &depth_attachment, _resolution);

    vkCmdBeginRendering(frame->cbuffer, &rendering_info);

    // draw_nodes(frame);

    /* imgui rendering */
    // ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame->cbuffer);

    vkCmdEndRendering(frame->cbuffer);

    /* transition image format for transfering */
    vk_cmd::vk_img_layout_transition(
        frame->cbuffer, _target.img, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _transfer_index);

    vk_cmd::vk_img_layout_transition(
        frame->cbuffer, _swapchain_imgs[_img_index], VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _transfer_index);

    // VkImageBlit region = {};
    // region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // region.srcSubresource.mipLevel = 0;
    // region.srcSubresource.baseArrayLayer = 0;
    // region.srcSubresource.layerCount = 1;
    // region.srcOffsets[1] = VkOffset3D{(int)_resolution.width, (int)_resolution.height, 1};
    // region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // region.dstSubresource.mipLevel = 0;
    // region.dstSubresource.baseArrayLayer = 0;
    // region.dstSubresource.layerCount = 1;
    // region.dstOffsets[1] =
    //     VkOffset3D{(int)_window_extent.width, (int)_window_extent.height, 1};

    // vkCmdBlitImage(frame->cbuffer, _target.img,
    //                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchain_imgs[_img_index],
    //                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

    /* only apply to window extent == resolution */
    vk_cmd::vk_img_copy(frame->cbuffer,
                        VkExtent3D{_window_extent.width, _window_extent.height, 1},
                        _target.img, _swapchain_imgs[_img_index]);

    /* transition image format for presenting */
    vk_cmd::vk_img_layout_transition(frame->cbuffer, _swapchain_imgs[_img_index],
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, _gfx_index);

    VK_CHECK(vkEndCommandBuffer(frame->cbuffer));

    /* submit present queue */
    VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submit_info = vk_boiler::submit_info(
        &frame->cbuffer, &frame->present_sem, &frame->sumbit_sem, &pipeline_stage_flags);

    VK_CHECK(vkQueueSubmit(_gfx_queue, 1, &submit_info, frame->fence));

    VkPresentInfoKHR present_info =
        vk_boiler::present_info(&_swapchain, &frame->sumbit_sem, &_img_index);

    vkQueuePresentKHR(_gfx_queue, &present_info);

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
            vkCmdBindPipeline(frame->cbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              _gfx_pipeline);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(frame->cbuffer, 0, 1, &mesh->vertex_buffer.buffer,
                                   &offset);

            vkCmdBindIndexBuffer(frame->cbuffer, mesh->index_buffer.buffer, 0,
                                 VK_INDEX_TYPE_UINT16);

            render_mat mat;
            mat.view = _vk_camera.get_view_mat();
            mat.proj = _vk_camera.get_proj_mat();
            mat.proj[1][1] *= -1;
            mat.model = node->transform_mat;

            void *data;
            vmaMapMemory(_allocator, _render_mat_buffer.allocation, &data);
            std::memcpy((char *)data + i * pad_uniform_buffer_size(sizeof(render_mat)),
                        &mat, sizeof(render_mat));
            vmaUnmapMemory(_allocator, _render_mat_buffer.allocation);

            std::vector<VkDescriptorSet> sets = {
                _render_mat_set,
                mesh->texture_set,
            };
            uint32_t doffset = i * pad_uniform_buffer_size(sizeof(render_mat));
            vkCmdBindDescriptorSets(frame->cbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    _gfx_pipeline_layout, 0, sets.size(), sets.data(), 1,
                                    &doffset);

            vkCmdDrawIndexed(frame->cbuffer, mesh->indices.size(), 1, 0, 0, 0);
        }
    }
}

void vk_engine::cleanup()
{
    vkDeviceWaitIdle(_device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (_is_initialized)
        deletion_queue.flush();
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

    // std::cout << "draw " << triangles << " triangels" << std::endl;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    auto input = std::async([&]() {
        while (!bquit) {
            const uint8_t *state = SDL_GetKeyboardState(NULL);

            float ms = (SDL_GetTicksNS() - _last_frame) / 1000000.f;

            if (SDL_GetRelativeMouseMode()) {
                if (state[SDL_SCANCODE_W])
                    _vk_camera.w(ms);

                if (state[SDL_SCANCODE_A])
                    _vk_camera.a(ms);

                if (state[SDL_SCANCODE_S])
                    _vk_camera.s(ms);

                if (state[SDL_SCANCODE_D])
                    _vk_camera.d(ms);

                if (state[SDL_SCANCODE_SPACE])
                    _vk_camera.space(ms);

                if (state[SDL_SCANCODE_LCTRL])
                    _vk_camera.ctrl(ms);

                float x, y;
                SDL_GetRelativeMouseState(&x, &y);
                _vk_camera.motion(x, y);
            }

            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_EVENT_QUIT)
                    bquit = true;

                if (e.type == SDL_EVENT_KEY_DOWN)
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                        bquit = true;

                if (e.type == SDL_EVENT_KEY_DOWN) {
                    if (e.key.keysym.sym == SDLK_TAB) {
                        if (SDL_GetRelativeMouseMode())
                            SDL_SetRelativeMouseMode(SDL_FALSE);
                        else
                            SDL_SetRelativeMouseMode(SDL_TRUE);
                    }
                } else if (!SDL_GetRelativeMouseMode())
                    ImGui_ImplSDL3_ProcessEvent(&e);
            }

            _last_frame = SDL_GetTicksNS();
        }
    });

    while (!bquit) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        draw();
    }
}

void vk_engine::imgui_init()
{
    /* Setup Dear ImGui context */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    VkPipelineRenderingCreateInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.pNext = nullptr;
    // rendering_info.viewMask = ;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &_format;
    rendering_info.depthAttachmentFormat = _depth_img.format;
    // rendering_info.stencilAttachmentFormat = ;

    /* Setup Platform/Renderer backends */
    ImGui_ImplSDL3_InitForVulkan(_window);
    ImGui_ImplVulkan_InitInfo imgui_init_info = {};
    imgui_init_info.Instance = _instance;
    imgui_init_info.PhysicalDevice = _physical_device;
    imgui_init_info.Device = _device;
    imgui_init_info.QueueFamily = _gfx_index;
    imgui_init_info.Queue = _gfx_queue;
    imgui_init_info.DescriptorPool = _descriptor_pool;
    imgui_init_info.MinImageCount = 2;
    imgui_init_info.ImageCount = 2;
    imgui_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imgui_init_info.UseDynamicRendering = true;
    imgui_init_info.PipelineRenderingCreateInfo = rendering_info;
    ImGui_ImplVulkan_Init(&imgui_init_info);
    ImGui_ImplVulkan_CreateFontsTexture();
    ImGui_ImplVulkan_DestroyFontsTexture();
}