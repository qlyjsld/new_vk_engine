#include "vk_engine.h"

#include <cstring>

#include <SDL3/SDL.h>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_pipeline.h"

/*
    Each of the functions below initialize a compute shader
    for running, follwing the structure:

        comp_allocator allocator(_device, _allocator);

        allocator.create_img(..., img_name);
        allocator.create_buffer(..., buffer_name);
        allocator.load_img(img_name, ...);

    comp_allocator has static class member for storing buffers and images
    in unordered_map with their name as key, it is common to share resources
    within multiple shaders. By default, _target, "target" is the framebuffer
    we draw to.

        std::vector<descriptor> descriptors = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, img_name},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, buffer_name},
        };

        cs compute_shader_example(allocator,
                                descriptors,
                                "../shaders/example.spv",
                                _min_buffer_alignment);

    Descriptors binding is done with vector of "descriptor" which is std::pair
   of <VkDescriptorType, std::string>, provided the name and arguments, create
   class cs. Then start building pipeline using info from struct cs and
   comp_allocator.

        PipelineBuilder pb = {};
        pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
            VK_SHADER_STAGE_COMPUTE_BIT, compute_shader_example.module));

        pb.build_comp(...);

    Finally, add draw commands. At this point, you have mutiple options, you
   have to run cc_init(...) the first time. After that, you could push_back(...)
   to have it executed in the main loop, or call comp_immediate_submit(...) to
   execute immediately, the latter one is ofter used for preparing texture or
   data used later.

        compute_shader_example.draw = [=](VkCommandBuffer cmd_buffer, cs *cs) {
            vkCmdBindPipeline(...);
            vkCmdBindDescriptorSets(...);
            vkCmdDispatch(...);
        };

        cs::cc_init(_comp_index, _device);
        cs::push_back(compute_shader_example);
        cs::comp_immediate_submit(_device, _queue, &compute_shader_example);
*/

void vk_engine::cloudtex_shader_init()
{
    uint32_t cloudtex_size = 128;

    uint32_t id = _comp_allocator.create_img(
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VkExtent3D{cloudtex_size, cloudtex_size, cloudtex_size},
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_STORAGE_BIT, 0, "cloudtex");

    _comp_allocator.create_buffer(pad_uniform_buffer_size(sizeof(float)),
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                                  "size");

    /* match set binding */
    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloudtex"},
    };

    cs cloudtex(&_comp_allocator, descriptors, "../shaders/cloudtex.comp.spv",
                _min_buffer_alignment);

    /* build pipeline */
    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, cloudtex.module));

    std::vector<VkPushConstantRange> push_constants = {};
    pb.build_comp(_device, push_constants, &cloudtex);

    immediate_draw(
        [&, cloudtex, cloudtex_size, id](VkCommandBuffer cmd_buffer) {
            vk_cmd::vk_img_layout_transition(
                cmd_buffer, _comp_allocator.imgs[id].img,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                _family_index);

            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                              cloudtex.pipeline);

            vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    cloudtex.pipeline_layout, 0, 1,
                                    &cloudtex.desc_set, 0, nullptr);

            vkCmdDispatch(cmd_buffer, cloudtex_size / 8, cloudtex_size / 8,
                          cloudtex_size / 8);
        },
        _queue);
}

void vk_engine::weather_shader_init()
{
    uint32_t weather_size = 512;

    uint32_t id = _comp_allocator.create_img(
        VK_FORMAT_R16_SFLOAT, VkExtent3D{weather_size, weather_size, 1},
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_STORAGE_BIT, 0, "weather");

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "weather"},
    };

    cs weather(&_comp_allocator, descriptors, "../shaders/weather.comp.spv",
               _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, weather.module));

    VkPushConstantRange u_time_pc = {};
    u_time_pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time_pc.offset = 0;
    u_time_pc.size = sizeof(float);

    std::vector<VkPushConstantRange> push_constants = {u_time_pc};
    pb.build_comp(_device, push_constants, &weather);

    cs_draw.push_back([&, weather, weather_size,
                       id](VkCommandBuffer cmd_buffer) {
        vk_cmd::vk_img_layout_transition(
            cmd_buffer, _comp_allocator.imgs[id].img, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL, _family_index);

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          weather.pipeline);

        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                weather.pipeline_layout, 0, 1,
                                &weather.desc_set, 0, nullptr);

        u_time = SDL_GetTicks() / 10000.f;
        vkCmdPushConstants(cmd_buffer, weather.pipeline_layout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float),
                           &u_time);

        vkCmdDispatch(cmd_buffer, weather_size / 8, weather_size / 8, 1);
    });
}

void vk_engine::cloud_shader_init()
{
    uint32_t cloud_id = _comp_allocator.create_buffer(
        pad_uniform_buffer_size(sizeof(cloud_data)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "cloud");

    _cloud_data.type = .6f;
    _cloud_data.freq = .2f;
    _cloud_data.ambient = .6f;
    _cloud_data.sigma_a = 0.f;
    _cloud_data.sigma_s = .2f;
    _cloud_data.step = 1.3f;
    _cloud_data.max_steps = 64;
    _cloud_data.cutoff = .5f;
    _cloud_data.density = 1.f;
    _cloud_data.sun_color = glm::vec3(.99f, .36f, .32f);
    _cloud_data.sky_color = glm::vec3(.98f, .83f, .64f);

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloudtex"},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "weather"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "cloud"},
    };

    cs cloud(&_comp_allocator, descriptors, "../shaders/cloud.comp.spv",
             _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, cloud.module));

    std::vector<VkPushConstantRange> push_constants = {};
    pb.build_comp(_device, push_constants, &cloud);

    uint32_t camera_id = _comp_allocator.get_buffer_id("camera");

    cs_draw.push_back([&, cloud, camera_id,
                       cloud_id](VkCommandBuffer cmd_buffer) {
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          cloud.pipeline);

        _camera_data.pos = _vk_camera.get_pos();
        _camera_data.fov = _vk_camera.get_fov();
        _camera_data.dir = _vk_camera.get_dir();
        _camera_data.width = _resolution.width;
        _camera_data.left = _vk_camera.get_left();
        _camera_data.height = _resolution.height;

        void *data;
        vmaMapMemory(_allocator, _comp_allocator.buffers[camera_id].allocation,
                     &data);
        std::memcpy(data, &_camera_data,
                    pad_uniform_buffer_size(sizeof(camera_data)));
        vmaUnmapMemory(_allocator,
                       _comp_allocator.buffers[camera_id].allocation);

        vmaMapMemory(_allocator, _comp_allocator.buffers[cloud_id].allocation,
                     &data);
        std::memcpy(data, &_cloud_data,
                    pad_uniform_buffer_size(sizeof(cloud_data)));
        vmaUnmapMemory(_allocator,
                       _comp_allocator.buffers[cloud_id].allocation);

        std::vector<uint32_t> doffsets = {0, 0};
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cloud.pipeline_layout, 0, 1, &cloud.desc_set,
                                doffsets.size(), doffsets.data());

        vkCmdDispatch(cmd_buffer, _resolution.width / 8, _resolution.height / 8,
                      1);
    });
}
