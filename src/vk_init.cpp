#include "vk_init.h"
#include "vk_engine.h"

#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

VkCommandPoolCreateInfo vk_init::vk_create_cmd_pool_info(uint32_t queue_family_index)
{
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = queue_family_index;
    return cmd_pool_info;
}

VkCommandBufferAllocateInfo
vk_init::vk_create_cmd_buffer_allocate_info(uint32_t cmd_buffer_count,
                                            VkCommandPool cmd_pool)
{
    VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
    cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_allocate_info.pNext = nullptr;
    cmd_buffer_allocate_info.commandPool = cmd_pool;
    cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_allocate_info.commandBufferCount = cmd_buffer_count;
    return cmd_buffer_allocate_info;
}

VkFenceCreateInfo vk_init::vk_create_fence_info(bool signaled)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    if (signaled)
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return fence_info;
}

VkSemaphoreCreateInfo vk_init::vk_create_sem_info()
{
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.pNext = nullptr;
    return sem_info;
}

VkRenderingAttachmentInfo
vk_init::vk_create_rendering_attachment_info(VkImageView img_view, VkImageLayout layout,
                                             VkClearValue clear_value)
{
    VkRenderingAttachmentInfo attachment_info = {};
    attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment_info.pNext = nullptr;
    attachment_info.imageView = img_view;
    attachment_info.imageLayout = layout;
    // attachment_info.resolveMode = ;
    // attachment_info.resolveImageView = ;
    // attachment_info.resolveImageLayout = ;
    attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.clearValue = clear_value;
    return attachment_info;
}

VkRenderingInfo
vk_init::vk_create_rendering_info(VkRenderingAttachmentInfo *color_attachments,
                                  VkRenderingAttachmentInfo *depth_attachment,
                                  VkExtent2D extent)
{
    VkRect2D render_area = {};
    render_area.offset = VkOffset2D{0, 0};
    render_area.extent = extent;

    VkRenderingInfo rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.pNext = nullptr;
    rendering_info.flags = 0;
    rendering_info.renderArea = render_area;
    rendering_info.layerCount = 1;
    rendering_info.viewMask = 0;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = color_attachments;
    rendering_info.pDepthAttachment = depth_attachment;
    rendering_info.pStencilAttachment = nullptr;
    return rendering_info;
}

VkCommandBufferBeginInfo vk_init::vk_create_cmd_buffer_begin_info()
{
    VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
    cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buffer_begin_info.pNext = nullptr;
    cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmd_buffer_begin_info.pInheritanceInfo = nullptr;
    return cmd_buffer_begin_info;
}

VkImageSubresourceRange vk_init::vk_create_subresource_range(VkImageAspectFlags aspect)
{
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = aspect;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    return subresource_range;
}

VkImageMemoryBarrier vk_init::vk_create_img_mem_barrier()
{
    VkImageMemoryBarrier img_mem_barrier = {};
    return img_mem_barrier;
}

VkSubmitInfo vk_init::vk_create_submit_info(VkCommandBuffer *cmd_buffer,
                                            VkSemaphore *wait_sem,
                                            VkSemaphore *signal_sem,
                                            VkPipelineStageFlags *pipeline_stage_flags)
{
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_sem;
    submit_info.pWaitDstStageMask = pipeline_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_sem;
    return submit_info;
}

VkPresentInfoKHR vk_init::vk_create_present_info(VkSwapchainKHR *swapchain,
                                                 VkSemaphore *sem, uint32_t *img_indices)
{
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = sem;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchain;
    present_info.pImageIndices = img_indices;
    present_info.pResults = nullptr;
    return present_info;
}

VkPipelineShaderStageCreateInfo
vk_init::vk_create_shader_stage_info(VkShaderStageFlagBits stage,
                                     VkShaderModule shader_module)
{
    VkPipelineShaderStageCreateInfo shader_stage_info = {};
    shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.pNext = nullptr;
    // shader_stage_info.flags = ;
    shader_stage_info.stage = stage;
    shader_stage_info.module = shader_module;
    shader_stage_info.pName = "main";
    // shader_stage_info.pSpecializationInfo = ;
    return shader_stage_info;
}

VkPipelineVertexInputStateCreateInfo vk_init::vk_create_vertex_input_state_info()
{
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pNext = nullptr;
    // vertex_input_state_info.flags = ;
    vertex_input_state_info.vertexBindingDescriptionCount = 0;
    vertex_input_state_info.pVertexBindingDescriptions = nullptr;
    vertex_input_state_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_info.pVertexAttributeDescriptions = nullptr;
    return vertex_input_state_info;
}

VkPipelineInputAssemblyStateCreateInfo
vk_init::vk_create_input_asm_state_info(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo input_asm_state_info = {};
    input_asm_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_state_info.pNext = nullptr;
    // input_asm_state_info.flags = ;
    input_asm_state_info.topology = topology;
    input_asm_state_info.primitiveRestartEnable = VK_FALSE;
    return input_asm_state_info;
}

VkPipelineRasterizationStateCreateInfo
vk_init::vk_create_rasterization_state_info(VkPolygonMode polygon_mode)
{
    VkPipelineRasterizationStateCreateInfo rasterization_state_info = {};
    rasterization_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.pNext = nullptr;
    // rasterization_state_info.flags = ;
    rasterization_state_info.depthClampEnable = VK_FALSE;
    rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_info.polygonMode = polygon_mode;
    rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = VK_FALSE;
    // rasterization_state_info.depthBiasConstantFactor = ;
    // rasterization_state_info.depthBiasClamp = ;
    // rasterization_state_info.depthBiasSlopeFactor = ;
    rasterization_state_info.lineWidth = 1.f;
    return rasterization_state_info;
}

VkPipelineColorBlendAttachmentState vk_init::vk_create_color_blend_attachment_state()
{
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    // color_blend_attachment_state.srcColorBlendFactor = ;
    // color_blend_attachment_state.dstColorBlendFactor = ;
    // color_blend_attachment_state.colorBlendOp = ;
    // color_blend_attachment_state.srcAlphaBlendFactor = ;
    // color_blend_attachment_state.dstAlphaBlendFactor = ;
    // color_blend_attachment_state.alphaBlendOp = ;
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    return color_blend_attachment_state;
}

VkPipelineMultisampleStateCreateInfo vk_init::vk_create_multisample_state_info()
{
    VkPipelineMultisampleStateCreateInfo multisample_state_info = {};
    multisample_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.pNext = nullptr;
    // multisample_state_info.flags = ;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    // multisample_state_info.minSampleShading = ;
    multisample_state_info.pSampleMask = nullptr;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;
    return multisample_state_info;
}

VkPipelineLayoutCreateInfo
vk_init::vk_create_pipeline_layout_info(std::vector<VkDescriptorSetLayout> &layouts)
{
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    // pipeline_layout_info.flags = ;
    pipeline_layout_info.setLayoutCount = layouts.size();
    pipeline_layout_info.pSetLayouts = layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    return pipeline_layout_info;
}

VkImageCreateInfo vk_init::vk_create_image_info(VkFormat format, VkExtent3D extent,
                                                VkImageUsageFlags usage)
{
    VkImageCreateInfo img_info = {};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.pNext = nullptr;
    // img_info.flags = ;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.format = format;
    img_info.extent = extent;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = usage;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // img_info.queueFamilyIndexCount = ;
    // img_info.pQueueFamilyIndices = ;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return img_info;
}

VkImageViewCreateInfo vk_init::vk_create_image_view_info(VkImageAspectFlags aspect,
                                                         VkImage img, VkFormat format)
{
    VkImageSubresourceRange subresource_range = vk_create_subresource_range(aspect);
    VkImageViewCreateInfo img_view_info = {};
    img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.pNext = nullptr;
    // img_view_info.flags = ;
    img_view_info.image = img;
    img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_info.format = format;
    // img_view_info.components = ;
    img_view_info.subresourceRange = subresource_range;
    return img_view_info;
}

VkPipelineDepthStencilStateCreateInfo vk_init::vk_create_depth_stencil_state_info()
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
    depth_stencil_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    // depth_stencil_state_info.flags = ;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;
    // depth_stencil_state_info.front = ;
    // depth_stencil_state_info.back = ;
    // depth_stencil_state_info.minDepthBounds = ;
    // depth_stencil_state_info.maxDepthBounds = ;
    return depth_stencil_state_info;
}

VkDescriptorPoolCreateInfo
vk_init::vk_create_descriptor_pool_info(uint32_t pool_size_count,
                                        VkDescriptorPoolSize *desc_pool_sizes)
{
    VkDescriptorPoolCreateInfo desc_pool_info = {};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.pNext = nullptr;
    // desc_pool_info.flags = ;
    desc_pool_info.maxSets = 1024;
    desc_pool_info.poolSizeCount = pool_size_count;
    desc_pool_info.pPoolSizes = desc_pool_sizes;
    return desc_pool_info;
}

VkDescriptorSetLayoutCreateInfo vk_init::vk_create_descriptor_set_layout_info(
    uint32_t binding_count, VkDescriptorSetLayoutBinding *desc_set_layout_bindings)
{
    VkDescriptorSetLayoutCreateInfo desc_set_layout_info = {};
    desc_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc_set_layout_info.pNext = nullptr;
    // desc_set_layout_info.flags = ;
    desc_set_layout_info.bindingCount = binding_count;
    desc_set_layout_info.pBindings = desc_set_layout_bindings;
    return desc_set_layout_info;
}

VkDescriptorSetAllocateInfo
vk_init::vk_allocate_descriptor_set_info(VkDescriptorPool desc_pool,
                                         VkDescriptorSetLayout *desc_set_layout)
{
    VkDescriptorSetAllocateInfo desc_set_allocate_info = {};
    desc_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_set_allocate_info.pNext = nullptr;
    desc_set_allocate_info.descriptorPool = desc_pool;
    desc_set_allocate_info.descriptorSetCount = 1;
    desc_set_allocate_info.pSetLayouts = desc_set_layout;
    return desc_set_allocate_info;
}

VkWriteDescriptorSet
vk_init::vk_create_write_descriptor_set(VkDescriptorBufferInfo *desc_buffer_info,
                                        VkDescriptorSet set, VkDescriptorType type)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = set;
    write_set.dstBinding = 0;
    write_set.descriptorCount = 1;
    write_set.descriptorType = type;
    write_set.pBufferInfo = desc_buffer_info;
    return write_set;
}

VkWriteDescriptorSet
vk_init::vk_create_write_descriptor_set(VkDescriptorImageInfo *desc_buffer_info,
                                        VkDescriptorSet set, VkDescriptorType type)
{
    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = nullptr;
    write_set.dstSet = set;
    write_set.dstBinding = 0;
    write_set.descriptorCount = 1;
    write_set.descriptorType = type;
    write_set.pImageInfo = desc_buffer_info;
    return write_set;
}

VkBufferImageCopy vk_init::vk_create_buffer_image_copy(VkExtent3D extent)
{
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
    return region;
}

VkSamplerCreateInfo vk_init::vk_create_sampler_info()
{
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    return sampler_info;
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
