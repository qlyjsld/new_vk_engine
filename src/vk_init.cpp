#include "vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

#include "vk_boiler.h"
#include "vk_type.h"

void vk_engine::device_init()
{
    // Create Instance
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("vk_engine")
                        .require_api_version(VKB_VK_API_VERSION_1_3)
#ifndef NDEBUG
                        .request_validation_layers(true)
                        .use_default_debug_messenger()
#endif
                        .build();

    if (!inst_ret) {
        std::cerr << "failed to create vulkan instance: "
                  << inst_ret.error().message() << std::endl;
        abort();
    }

    auto instance = inst_ret.value();
    _instance = instance.instance;
    _debug_utils_messenger = instance.debug_messenger;

    deletion_queue.push_back([=]() {
        vkb::destroy_debug_utils_messenger(_instance, _debug_utils_messenger,
                                           nullptr);
        vkDestroyInstance(_instance, nullptr);
    });

    // create surface
    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    deletion_queue.push_back(
        [=]() { vkDestroySurfaceKHR(_instance, _surface, nullptr); });

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {};
    dynamic_rendering_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamic_rendering_features.pNext = nullptr;
    dynamic_rendering_features.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {};
    mesh_shader_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    mesh_shader_features.pNext = nullptr;
    mesh_shader_features.taskShader = VK_FALSE;
    mesh_shader_features.meshShader = VK_TRUE;
    mesh_shader_features.multiviewMeshShader = VK_FALSE;
    mesh_shader_features.primitiveFragmentShadingRateMeshShader = VK_FALSE;
    mesh_shader_features.meshShaderQueries = VK_FALSE;

    // create physical device
    vkb::PhysicalDeviceSelector selector(instance);
    auto phys_ret =
        selector.add_required_extension_features(mesh_shader_features)
            .add_required_extension_features(dynamic_rendering_features)
            .set_surface(_surface)
            .select();

    if (!phys_ret) {
        std::cerr << "failed to find suitable physical device: "
                  << phys_ret.error().message() << std::endl;
        abort();
    }

    auto physical_device = phys_ret.value();
    physical_device.enable_extension_if_present("VK_EXT_mesh_shader");
    _physical_device = physical_device.physical_device;
    _min_buffer_alignment =
        physical_device.properties.limits.minUniformBufferOffsetAlignment;

    // create device
    vkb::DeviceBuilder device_builder(physical_device);
    auto dev_ret = device_builder.build();

    if (!dev_ret) {
        std::cerr << "failed to create vulkan device: "
                  << dev_ret.error().message() << std::endl;
        abort();
    }

    vkb::Device device = dev_ret.value();
    _device = device.device;

    deletion_queue.push_back([=]() { vkDestroyDevice(_device, nullptr); });

    auto queue_ret = device.get_queue(vkb::QueueType::graphics);

    if (!queue_ret) {
        std::cerr << "failed to get compute queue:" << queue_ret.error()
                  << std::endl;
        abort();
    }

    _queue = queue_ret.value();
}

void vk_engine::vma_init()
{
    VmaAllocatorCreateInfo vma_allocator_info = {};
    vma_allocator_info.physicalDevice = _physical_device;
    vma_allocator_info.device = _device;
    vma_allocator_info.instance = _instance;
    vma_allocator_info.pVulkanFunctions = &vma_vulkan_func;
    vmaCreateAllocator(&vma_allocator_info, &_allocator);

    deletion_queue.push_back([=]() { vmaDestroyAllocator(_allocator); });
}

void vk_engine::swapchain_init()
{
    vkb::SwapchainBuilder vkb_swapchain_builder{_physical_device, _device,
                                                _surface};
    vkb::Swapchain vkb_swapchain =
        vkb_swapchain_builder
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(_window_extent.width, _window_extent.height)
            .set_desired_format(VkSurfaceFormatKHR{_format, _colorspace})
            .set_image_usage_flags(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            .build()
            .value();

    _swapchain = vkb_swapchain.swapchain;
    _swapchain_imgs = vkb_swapchain.get_images().value();
    _swapchain_img_views = vkb_swapchain.get_image_views().value();

    deletion_queue.push_back(
        [=]() { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });

    for (uint32_t i = 0; i < _swapchain_img_views.size(); i++)
        deletion_queue.push_back([=]() {
            vkDestroyImageView(_device, _swapchain_img_views[i], nullptr);
        });

    _depth_img.format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo img_info = vk_boiler::img_create_info(
        _depth_img.format, VkExtent3D{_resolution.width, _resolution.height, 1},
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &alloc_info, &_depth_img.img,
                            &_depth_img.allocation, nullptr));

    deletion_queue.push_back([=]() {
        vmaDestroyImage(_allocator, _depth_img.img, _depth_img.allocation);
    });

    VkImageViewCreateInfo img_view_info = vk_boiler::img_view_create_info(
        VK_IMAGE_ASPECT_DEPTH_BIT, _depth_img.img,
        VkExtent3D{_resolution.width, _resolution.height, 1},
        _depth_img.format);

    VK_CHECK(vkCreateImageView(_device, &img_view_info, nullptr,
                               &_depth_img.img_view));

    deletion_queue.push_back(
        [=]() { vkDestroyImageView(_device, _depth_img.img_view, nullptr); });

    /* create img target for rendering */
    VkExtent3D extent = {};
    extent.width = _resolution.width;
    extent.height = _resolution.height;
    extent.depth = 1;

    create_img(_format, extent, VK_IMAGE_ASPECT_COLOR_BIT,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               0, &_target);

    VkSamplerCreateInfo sampler_info = vk_boiler::sampler_create_info();
    VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &_sampler));
    deletion_queue.push_back(
        [=]() { vkDestroySampler(_device, _sampler, nullptr); });
}

void vk_engine::cmd_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkCommandPoolCreateInfo cmd_pool_info =
            vk_boiler::cmd_pool_create_info(_family_index);

        VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr,
                                     &_frames[i].cmd_pool));

        deletion_queue.push_back([=]() {
            vkDestroyCommandPool(_device, _frames[i].cmd_pool, nullptr);
        });

        VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
            vk_boiler::cmd_buffer_allocate_info(1, _frames[i].cmd_pool);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                          &_frames[i].cmd_buffer));
    }

    VkCommandPoolCreateInfo cmd_pool_info =
        vk_boiler::cmd_pool_create_info(_family_index);

    VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr,
                                 &_immed_context.cmd_pool));

    deletion_queue.push_back([=]() {
        vkDestroyCommandPool(_device, _immed_context.cmd_pool, nullptr);
    });

    VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
        vk_boiler::cmd_buffer_allocate_info(1, _immed_context.cmd_pool);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                      &_immed_context.cmd_buffer));
}

void vk_engine::sync_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkFenceCreateInfo fence_info = vk_boiler::fence_create_info(true);

        VK_CHECK(
            vkCreateFence(_device, &fence_info, nullptr, &_frames[i].fence));

        deletion_queue.push_back(
            [=]() { vkDestroyFence(_device, _frames[i].fence, nullptr); });

        VkSemaphoreCreateInfo sem_info = vk_boiler::sem_create_info();

        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr,
                                   &_frames[i].sumbit_sem));

        deletion_queue.push_back([=]() {
            vkDestroySemaphore(_device, _frames[i].sumbit_sem, nullptr);
        });

        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr,
                                   &_frames[i].present_sem));

        deletion_queue.push_back([=]() {
            vkDestroySemaphore(_device, _frames[i].present_sem, nullptr);
        });
    }

    VkFenceCreateInfo fence_info = vk_boiler::fence_create_info(false);

    VK_CHECK(
        vkCreateFence(_device, &fence_info, nullptr, &_immed_context.fence));

    deletion_queue.push_back(
        [=]() { vkDestroyFence(_device, _immed_context.fence, nullptr); });
}

void vk_engine::desc_init()
{
    std::vector<VkDescriptorPoolSize> desc_pool_sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };

    VkDescriptorPoolCreateInfo desc_pool_info =
        vk_boiler::descriptor_pool_create_info(desc_pool_sizes.size(),
                                               desc_pool_sizes.data());

    VK_CHECK(
        vkCreateDescriptorPool(_device, &desc_pool_info, nullptr, &_desc_pool));

    deletion_queue.push_back(
        [=]() { vkDestroyDescriptorPool(_device, _desc_pool, nullptr); });

    /* render mat layout and set */
    VkDescriptorSetLayoutCreateInfo render_mat_layout_info =
        vk_boiler::descriptor_set_layout_create_info(
            std::vector<VkDescriptorType>{
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            },
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_MESH_BIT_EXT);

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &render_mat_layout_info,
                                         nullptr, &_render_mat_layout));

    deletion_queue.push_back([=]() {
        vkDestroyDescriptorSetLayout(_device, _render_mat_layout, nullptr);
    });

    VkDescriptorSetAllocateInfo desc_set_allocate_info =
        vk_boiler::descriptor_set_allocate_info(_desc_pool,
                                                &_render_mat_layout);

    VK_CHECK(vkAllocateDescriptorSets(_device, &desc_set_allocate_info,
                                      &_render_mat_set));

    /* texture layout */
    VkDescriptorSetLayoutCreateInfo texture_data_layout_info =
        vk_boiler::descriptor_set_layout_create_info(
            std::vector<VkDescriptorType>{
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            },
            VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &texture_data_layout_info,
                                         nullptr, &_texture_layout));

    deletion_queue.push_back([=]() {
        vkDestroyDescriptorSetLayout(_device, _texture_layout, nullptr);
    });

    /* vertex layout */
    VkDescriptorSetLayoutCreateInfo vertex_data_layout_info =
        vk_boiler::descriptor_set_layout_create_info(
            std::vector<VkDescriptorType>{
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            },
            VK_SHADER_STAGE_MESH_BIT_EXT);

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &vertex_data_layout_info,
                                         nullptr, &_vertex_layout));

    deletion_queue.push_back([=]() {
        vkDestroyDescriptorSetLayout(_device, _vertex_layout, nullptr);
    });

    /* meshlet layout */
    VkDescriptorSetLayoutCreateInfo meshlet_layout_info =
        vk_boiler::descriptor_set_layout_create_info(
            std::vector<VkDescriptorType>{
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            },
            VK_SHADER_STAGE_MESH_BIT_EXT);

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &meshlet_layout_info, nullptr,
                                         &_meshlet_layout));

    deletion_queue.push_back([=]() {
        vkDestroyDescriptorSetLayout(_device, _meshlet_layout, nullptr);
    });
}