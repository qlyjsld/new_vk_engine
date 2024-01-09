#include "vk_engine.h"

#include <fstream>
#include <iostream>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "vk_cmd.h"
#include "vk_init.h"
#include "vk_mesh.h"
#include "vk_pipeline_builder.h"
#include "vk_type.h"

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
    upload_meshes(_meshes.data(), _meshes.size());
    upload_textures(_meshes.data(), _meshes.size());

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

    _deletion_queue.push_back([=]() {
        vkb::destroy_debug_utils_messenger(_instance, _debug_utils_messenger, nullptr);
        vkDestroyInstance(_instance, nullptr);
    });

    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    _deletion_queue.push_back(
        [=]() { vkDestroySurfaceKHR(_instance, _surface, nullptr); });

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

    std::cout << "minUniformBufferOffsetAlignment "
              << vkb_physical_device.properties.limits.minUniformBufferOffsetAlignment
              << std::endl;

    _minUniformBufferOffsetAlignment =
        vkb_physical_device.properties.limits.minUniformBufferOffsetAlignment;

    /* create vulkan device */
    vkb::DeviceBuilder vkb_device_builder{vkb_physical_device};
    vkb::Device vkb_device = vkb_device_builder.build().value();
    _device = vkb_device.device;

    _deletion_queue.push_back([=]() { vkDestroyDevice(_device, nullptr); });

    /* get queue for commands */
    _gfx_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _gfx_queue_family_index =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    _transfer_queue = vkb_device.get_queue(vkb::QueueType::transfer).value();
    _transfer_queue_family_index =
        vkb_device.get_queue_index(vkb::QueueType::transfer).value();
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

    _deletion_queue.push_back(
        [=]() { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });

    for (uint32_t i = 0; i < _swapchain_img_views.size(); i++)
        _deletion_queue.push_back(
            [=]() { vkDestroyImageView(_device, _swapchain_img_views[i], nullptr); });

    _depth_img.format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo img_info = vk_init::vk_create_image_info(
        _depth_img.format, VkExtent3D{_window_extent.width, _window_extent.height, 1},
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &alloc_info, &_depth_img.img,
                            &_depth_img.allocation, nullptr));

    _deletion_queue.push_back(
        [=]() { vmaDestroyImage(_allocator, _depth_img.img, _depth_img.allocation); });

    VkImageViewCreateInfo img_view_info = vk_init::vk_create_image_view_info(
        VK_IMAGE_ASPECT_DEPTH_BIT, _depth_img.img, _depth_img.format);

    VK_CHECK(vkCreateImageView(_device, &img_view_info, nullptr, &_depth_img.img_view));

    _deletion_queue.push_back(
        [=]() { vkDestroyImageView(_device, _depth_img.img_view, nullptr); });
}

void vk_engine::command_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkCommandPoolCreateInfo cmd_pool_info =
            vk_init::vk_create_cmd_pool_info(_gfx_queue_family_index);

        VK_CHECK(
            vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_frames[i].cmd_pool));

        _deletion_queue.push_back(
            [=]() { vkDestroyCommandPool(_device, _frames[i].cmd_pool, nullptr); });

        VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
            vk_init::vk_create_cmd_buffer_allocate_info(1, _frames[i].cmd_pool);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                          &_frames[i].cmd_buffer));
    }

    VkCommandPoolCreateInfo cmd_pool_info =
        vk_init::vk_create_cmd_pool_info(_transfer_queue_family_index);

    VK_CHECK(
        vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_upload_context.cmd_pool));

    _deletion_queue.push_back(
        [=]() { vkDestroyCommandPool(_device, _upload_context.cmd_pool, nullptr); });

    VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
        vk_init::vk_create_cmd_buffer_allocate_info(1, _upload_context.cmd_pool);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                      &_upload_context.cmd_buffer));
}

void vk_engine::sync_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkFenceCreateInfo fence_info = vk_init::vk_create_fence_info(true);

        VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_frames[i].fence));

        _deletion_queue.push_back(
            [=]() { vkDestroyFence(_device, _frames[i].fence, nullptr); });

        VkSemaphoreCreateInfo sem_info = vk_init::vk_create_sem_info();

        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr, &_frames[i].sumbit_sem));

        _deletion_queue.push_back(
            [=]() { vkDestroySemaphore(_device, _frames[i].sumbit_sem, nullptr); });

        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr, &_frames[i].present_sem));

        _deletion_queue.push_back(
            [=]() { vkDestroySemaphore(_device, _frames[i].present_sem, nullptr); });
    }

    VkFenceCreateInfo fence_info = vk_init::vk_create_fence_info(false);

    VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_upload_context.fence));

    _deletion_queue.push_back(
        [=]() { vkDestroyFence(_device, _upload_context.fence, nullptr); });
}

void vk_engine::descriptor_init()
{
    std::vector<VkDescriptorPoolSize> desc_pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 8},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}};

    VkDescriptorPoolCreateInfo desc_pool_info = vk_init::vk_create_descriptor_pool_info(
        desc_pool_sizes.size(), desc_pool_sizes.data());

    VK_CHECK(vkCreateDescriptorPool(_device, &desc_pool_info, nullptr, &_desc_pool));

    _deletion_queue.push_back(
        [=]() { vkDestroyDescriptorPool(_device, _desc_pool, nullptr); });

    VkDescriptorSetLayoutBinding desc_set_layout_binding_0 = {};
    desc_set_layout_binding_0.binding = 0;
    desc_set_layout_binding_0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    desc_set_layout_binding_0.descriptorCount = 1;
    desc_set_layout_binding_0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // desc_set_layout_binding.pImmutableSamplers = ;

    VkDescriptorSetLayoutBinding desc_set_layout_binding_1 = {};
    desc_set_layout_binding_1.binding = 1;
    desc_set_layout_binding_1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_set_layout_binding_1.descriptorCount = 1;
    desc_set_layout_binding_1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // desc_set_layout_binding.pImmutableSamplers = ;

    std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings = {
        desc_set_layout_binding_0, desc_set_layout_binding_1};

    VkDescriptorSetLayoutCreateInfo desc_set_layout_info =
        vk_init::vk_create_descriptor_set_layout_info(desc_set_layout_bindings.size(),
                                                      desc_set_layout_bindings.data());

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &desc_set_layout_info, nullptr,
                                         &_desc_set_layout));

    _deletion_queue.push_back(
        [=]() { vkDestroyDescriptorSetLayout(_device, _desc_set_layout, nullptr); });

    VkDescriptorSetAllocateInfo desc_set_allocate_info =
        vk_init::vk_allocate_descriptor_set_info(_desc_pool, &_desc_set_layout);

    VK_CHECK(vkAllocateDescriptorSets(_device, &desc_set_allocate_info, &_desc_set));
}

void vk_engine::pipeline_init()
{
    PipelineBuilder graphics_pipeline_builder = {};

    if (!load_shader_module("../shaders/.vert.spv", &_vert))
        std::cerr << "load vert failed" << std::endl;
    else
        std::cout << "vert loaded" << std::endl;

    _deletion_queue.push_back([=]() { vkDestroyShaderModule(_device, _vert, nullptr); });

    graphics_pipeline_builder._shader_stage_infos.push_back(
        vk_init::vk_create_shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, _vert));

    if (!load_shader_module("../shaders/.frag.spv", &_frag))
        std::cerr << "load frag failed" << std::endl;
    else
        std::cout << "frag loaded" << std::endl;

    _deletion_queue.push_back([=]() { vkDestroyShaderModule(_device, _frag, nullptr); });

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

    // VkPushConstantRange push_constant_range = {};
    // push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // push_constant_range.offset = 0;
    // push_constant_range.size = sizeof(mesh_push_constants);

    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &_desc_set_layout;
    // pipeline_layout_info.pushConstantRangeCount = 1;
    // pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                                    &graphics_pipeline_builder._pipeline_layout));

    _deletion_queue.push_back(
        [=]() { vkDestroyPipelineLayout(_device, _gfx_pipeline_layout, nullptr); });

    graphics_pipeline_builder.build(_device, &_swapchain_format, _depth_img.format);
    _gfx_pipeline = graphics_pipeline_builder.value();
    _gfx_pipeline_layout = graphics_pipeline_builder._pipeline_layout;

    _deletion_queue.push_back(
        [=]() { vkDestroyPipeline(_device, _gfx_pipeline, nullptr); });
}

void vk_engine::load_meshes()
{
    _meshes = load_from_gltf("/home/jay/Downloads/victorian_hallway.glb", _nodes);

    _mat_buffer = create_buffer(
        _nodes.size() * pad_uniform_buffer_size(sizeof(render_mat)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

    _deletion_queue.push_back([=]() {
        vmaDestroyBuffer(_allocator, _mat_buffer.buffer, _mat_buffer.allocation);
    });

    VkDescriptorBufferInfo desc_buffer_info = {};
    desc_buffer_info.buffer = _mat_buffer.buffer;
    desc_buffer_info.offset = 0;
    desc_buffer_info.range = sizeof(render_mat);

    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = _desc_set;
    write_set.dstBinding = 0;
    write_set.descriptorCount = 1;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write_set.pBufferInfo = &desc_buffer_info;

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
}

void vk_engine::upload_meshes(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh *mesh = &meshes[i];
        allocated_buffer staging_buffer;

        /* create vertex buffer */
        staging_buffer = create_buffer(mesh->vertices.size() * sizeof(vertex),
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

        void *data;
        vmaMapMemory(_allocator, staging_buffer.allocation, &data);
        std::memcpy(data, mesh->vertices.data(), mesh->vertices.size() * sizeof(vertex));
        vmaUnmapMemory(_allocator, staging_buffer.allocation);

        mesh->vertex_buffer = create_buffer(
            mesh->vertices.size() * sizeof(vertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

        _deletion_queue.push_back([=]() {
            vmaDestroyBuffer(_allocator, _meshes[i].vertex_buffer.buffer,
                             _meshes[i].vertex_buffer.allocation);
        });

        immediate_submit([=](VkCommandBuffer cmd_buffer) {
            VkBufferCopy region = {};
            region.size = mesh->vertices.size() * sizeof(vertex);
            vkCmdCopyBuffer(cmd_buffer, staging_buffer.buffer, mesh->vertex_buffer.buffer,
                            1, &region);
        });

        vmaDestroyBuffer(_allocator, staging_buffer.buffer, staging_buffer.allocation);

        /* create index buffer */
        staging_buffer = create_buffer(mesh->indices.size() * sizeof(uint16_t),
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

        vmaMapMemory(_allocator, staging_buffer.allocation, &data);
        std::memcpy(data, mesh->indices.data(), mesh->indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(_allocator, staging_buffer.allocation);

        mesh->index_buffer = create_buffer(
            mesh->indices.size() * sizeof(uint16_t),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

        _deletion_queue.push_back([=]() {
            vmaDestroyBuffer(_allocator, _meshes[i].index_buffer.buffer,
                             _meshes[i].index_buffer.allocation);
        });

        immediate_submit([=](VkCommandBuffer cmd_buffer) {
            VkBufferCopy region = {};
            region.size = mesh->indices.size() * sizeof(uint16_t);
            vkCmdCopyBuffer(cmd_buffer, staging_buffer.buffer, mesh->index_buffer.buffer,
                            1, &region);
        });

        vmaDestroyBuffer(_allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
}

void vk_engine::upload_textures(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh *mesh = &meshes[i];
        allocated_buffer staging_buffer;

        if (mesh->texture.size() != 0) {
            staging_buffer = create_buffer(mesh->texture.size() * sizeof(unsigned char),
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

            void *data;
            vmaMapMemory(_allocator, staging_buffer.allocation, &data);
            std::memcpy(data, mesh->texture.data(),
                        mesh->texture.size() * sizeof(unsigned char));
            vmaUnmapMemory(_allocator, staging_buffer.allocation);

            VkExtent3D extent = {};
            extent.width = mesh->texture_buffer.width;
            extent.height = mesh->texture_buffer.height;
            extent.depth = 1;

            mesh->texture_buffer = create_img(
                mesh->texture_buffer.format, extent, VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0);

            _deletion_queue.push_back([=]() {
                vkDestroyImageView(_device, _meshes[i].texture_buffer.img_view, nullptr);
                vmaDestroyImage(_allocator, _meshes[i].texture_buffer.img,
                                _meshes[i].texture_buffer.allocation);
            });

            immediate_submit([=](VkCommandBuffer cmd_buffer) {
                vk_cmd::vk_img_layout_transition(
                    cmd_buffer, mesh->texture_buffer.img, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _transfer_queue_family_index);

                VkBufferImageCopy region = {};
                region.bufferOffset = 0;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;
                region.imageOffset = VkOffset3D{0, 0, 0};
                region.imageExtent = extent;

                vkCmdCopyBufferToImage(cmd_buffer, staging_buffer.buffer,
                                       mesh->texture_buffer.img,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                vk_cmd::vk_img_layout_transition(cmd_buffer, mesh->texture_buffer.img,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                 _transfer_queue_family_index);
            });

            vmaDestroyBuffer(_allocator, staging_buffer.buffer,
                             staging_buffer.allocation);

            VkSamplerCreateInfo sampler_info = {};
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.pNext = nullptr;
            sampler_info.magFilter = VK_FILTER_NEAREST;
            sampler_info.minFilter = VK_FILTER_NEAREST;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

            VkSampler sampler;

            VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &sampler));

            _deletion_queue.push_back(
                [=]() { vkDestroySampler(_device, sampler, nullptr); });

            VkDescriptorImageInfo desc_img_info = {};
            desc_img_info.sampler = sampler;
            desc_img_info.imageView = mesh->texture_buffer.img_view;
            desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write_set = {};
            write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_set.pNext = nullptr;
            write_set.dstSet = _desc_set;
            write_set.dstBinding = 1;
            write_set.descriptorCount = 1;
            write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_set.pImageInfo = &desc_img_info;

            vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
        }
    }
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
                glm::perspective(glm::radians(_cam.fov), 1600.f / 900.f, .1f, 1024.0f);
            mat.proj[1][1] *= -1;
            mat.model = node->transform_mat;

            void *data;
            vmaMapMemory(_allocator, _mat_buffer.allocation, &data);
            std::memcpy((char *)data + i * pad_uniform_buffer_size(sizeof(render_mat)),
                        &mat, sizeof(render_mat));
            vmaUnmapMemory(_allocator, _mat_buffer.allocation);

            uint32_t doffset = i * pad_uniform_buffer_size(sizeof(render_mat));
            vkCmdBindDescriptorSets(frame->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    _gfx_pipeline_layout, 0, 1, &_desc_set, 1, &doffset);

            // mesh_push_constants push_constants;
            // push_constants.render_mat = mat.proj * mat.view * mat.model;

            // vkCmdPushConstants(frame->cmd_buffer, _gfx_pipeline_layout,
            //                    VK_SHADER_STAGE_VERTEX_BIT, 0,
            //                    sizeof(mesh_push_constants), &push_constants);

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
            _cam.rotate_yaw(_cam.sensitivity / 1000.f);
        }

        if (state[SDL_SCANCODE_E]) {
            _cam.rotate_yaw(-_cam.sensitivity / 1000.f);
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

void vk_engine::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&fs)
{
    /* prepare command buffer */
    VkCommandBufferBeginInfo cmd_buffer_begin_info =
        vk_init::vk_create_cmd_buffer_begin_info();

    /* begin command buffer recording */
    VK_CHECK(vkBeginCommandBuffer(_upload_context.cmd_buffer, &cmd_buffer_begin_info));

    fs(_upload_context.cmd_buffer);

    VK_CHECK(vkEndCommandBuffer(_upload_context.cmd_buffer));

    VkSubmitInfo submit_info = vk_init::vk_create_submit_info(&_upload_context.cmd_buffer,
                                                              nullptr, nullptr, nullptr);

    submit_info.waitSemaphoreCount = 0;
    submit_info.signalSemaphoreCount = 0;

    VK_CHECK(vkQueueSubmit(_transfer_queue, 1, &submit_info, _upload_context.fence));
    VK_CHECK(vkWaitForFences(_device, 1, &_upload_context.fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &_upload_context.fence));
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

allocated_buffer vk_engine::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                          VmaAllocationCreateFlags flags)
{
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    allocated_buffer buffer;

    VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vma_allocation_info,
                             &buffer.buffer, &buffer.allocation, nullptr));

    return buffer;
}

allocated_img vk_engine::create_img(VkFormat format, VkExtent3D extent,
                                    VkImageAspectFlags aspect, VkImageUsageFlags usage,
                                    VmaAllocationCreateFlags flags)
{
    VkImageCreateInfo img_info = vk_init::vk_create_image_info(format, extent, usage);

    VmaAllocationCreateInfo vma_allocation_info = {};
    vma_allocation_info.flags = flags;
    vma_allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    allocated_img img;
    img.format = format;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &vma_allocation_info, &img.img,
                            &img.allocation, nullptr));

    VkImageViewCreateInfo img_view_info =
        vk_init::vk_create_image_view_info(aspect, img.img, format);

    VK_CHECK(vkCreateImageView(_device, &img_view_info, nullptr, &img.img_view));

    return img;
}

size_t vk_engine::pad_uniform_buffer_size(size_t original_size)
{
    size_t aligned_size = original_size;
    if (_minUniformBufferOffsetAlignment > 0)
        aligned_size = (aligned_size + _minUniformBufferOffsetAlignment - 1) &
                       ~(_minUniformBufferOffsetAlignment - 1);
    return aligned_size;
}
