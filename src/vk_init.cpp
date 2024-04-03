#include "vk_engine.h"

#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

#include "vk_boiler.h"

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

    /* get queues for commands */
    _gfx_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _gfx_queue_family_index =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    _transfer_queue = vkb_device.get_queue(vkb::QueueType::transfer).value();
    _transfer_queue_family_index =
        vkb_device.get_queue_index(vkb::QueueType::transfer).value();

    _comp_queue = vkb_device.get_queue(vkb::QueueType::compute).value();
    _comp_queue_family_index =
        vkb_device.get_queue_index(vkb::QueueType::compute).value();
}

void vk_engine::vma_init()
{
    VmaAllocatorCreateInfo vma_allocator_info = {};
    vma_allocator_info.physicalDevice = _physical_device;
    vma_allocator_info.device = _device;
    vma_allocator_info.instance = _instance;
    vmaCreateAllocator(&vma_allocator_info, &_allocator);

    _deletion_queue.push_back([=]() { vmaDestroyAllocator(_allocator); });
}

void vk_engine::swapchain_init()
{
    vkb::SwapchainBuilder vkb_swapchain_builder{_physical_device, _device, _surface};
    vkb::Swapchain vkb_swapchain =
        vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(_window_extent.width, _window_extent.height)
            .set_image_usage_flags(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
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

    VkImageCreateInfo img_info = vk_boiler::img_create_info(
        _depth_img.format, VkExtent3D{_window_extent.width, _window_extent.height, 1},
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(_allocator, &img_info, &alloc_info, &_depth_img.img,
                            &_depth_img.allocation, nullptr));

    _deletion_queue.push_back(
        [=]() { vmaDestroyImage(_allocator, _depth_img.img, _depth_img.allocation); });

    VkImageViewCreateInfo img_view_info = vk_boiler::img_view_create_info(
        VK_IMAGE_ASPECT_DEPTH_BIT, _depth_img.img, _depth_img.format);

    VK_CHECK(vkCreateImageView(_device, &img_view_info, nullptr, &_depth_img.img_view));

    _deletion_queue.push_back(
        [=]() { vkDestroyImageView(_device, _depth_img.img_view, nullptr); });

    /* create img target for rendering and copying */
    {
        VkExtent3D extent = {};
        extent.width = _resolution.width;
        extent.height = _resolution.height;
        extent.depth = 1;

        _target = create_img(
            _format, extent, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

        _deletion_queue.push_back([=]() {
            vkDestroyImageView(_device, _target.img_view, nullptr);
            vmaDestroyImage(_allocator, _target.img, _target.allocation);
        });
    }

    {
        VkExtent3D extent = {};
        extent.width = _window_extent.width;
        extent.height = _window_extent.height;
        extent.depth = 1;

        _copy_to_swapchain =
            create_img(_format, extent, VK_IMAGE_ASPECT_COLOR_BIT,
                       VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0);

        _deletion_queue.push_back([=]() {
            vkDestroyImageView(_device, _copy_to_swapchain.img_view, nullptr);
            vmaDestroyImage(_allocator, _copy_to_swapchain.img,
                            _copy_to_swapchain.allocation);
        });
    }

    VkSamplerCreateInfo sampler_info = vk_boiler::sampler_create_info();
    VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &_sampler));
    _deletion_queue.push_back([=]() { vkDestroySampler(_device, _sampler, nullptr); });
}

void vk_engine::command_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkCommandPoolCreateInfo cmd_pool_info =
            vk_boiler::cmd_pool_create_info(_gfx_queue_family_index);

        VK_CHECK(
            vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_frames[i].cmd_pool));

        _deletion_queue.push_back(
            [=]() { vkDestroyCommandPool(_device, _frames[i].cmd_pool, nullptr); });

        VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
            vk_boiler::cmd_buffer_allocate_info(1, _frames[i].cmd_pool);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                          &_frames[i].cmd_buffer));
    }

    VkCommandPoolCreateInfo cmd_pool_info =
        vk_boiler::cmd_pool_create_info(_transfer_queue_family_index);

    VK_CHECK(
        vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_upload_context.cmd_pool));

    _deletion_queue.push_back(
        [=]() { vkDestroyCommandPool(_device, _upload_context.cmd_pool, nullptr); });

    VkCommandBufferAllocateInfo cmd_buffer_allocate_info =
        vk_boiler::cmd_buffer_allocate_info(1, _upload_context.cmd_pool);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_allocate_info,
                                      &_upload_context.cmd_buffer));
}

void vk_engine::sync_init()
{
    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        VkFenceCreateInfo fence_info = vk_boiler::fence_create_info(true);

        VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_frames[i].fence));

        _deletion_queue.push_back(
            [=]() { vkDestroyFence(_device, _frames[i].fence, nullptr); });

        VkSemaphoreCreateInfo sem_info = vk_boiler::sem_create_info();

        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr, &_frames[i].sumbit_sem));

        _deletion_queue.push_back(
            [=]() { vkDestroySemaphore(_device, _frames[i].sumbit_sem, nullptr); });

        VK_CHECK(vkCreateSemaphore(_device, &sem_info, nullptr, &_frames[i].present_sem));

        _deletion_queue.push_back(
            [=]() { vkDestroySemaphore(_device, _frames[i].present_sem, nullptr); });
    }

    VkFenceCreateInfo fence_info = vk_boiler::fence_create_info(false);

    VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_upload_context.fence));

    _deletion_queue.push_back(
        [=]() { vkDestroyFence(_device, _upload_context.fence, nullptr); });
}
