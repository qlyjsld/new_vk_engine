#include "vk_engine.h"

#include <cstring>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_comp.h"
#include "vk_pipeline.h"
#include "vk_type.h"

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();
    engine.skybox_init();
    engine.texture_init();
    engine.marching_init();
    engine.run();
    engine.cleanup();
    return 0;
}

void vk_engine::skybox_init()
{
    comp_allocator allocator(_device, _allocator);

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(glm::vec2)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "extent");

    glm::vec2 extent = glm::vec2{
        _resolution.width,
        _resolution.height,
    };

    allocated_buffer buffer = allocator.get_buffer("extent");

    void *data;
    vmaMapMemory(_allocator, buffer.allocation, &data);
    std::memcpy(data, &extent, sizeof(glm::vec2));
    vmaUnmapMemory(_allocator, buffer.allocation);

    allocator.load_img("target", _target);

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
    };

    cs skybox(allocator, descriptors, "../shaders/skybox.comp.spv",
              _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, skybox.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        skybox.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &skybox.pipeline_layout,
                  &skybox.pipeline);

    skybox.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        uint32_t doffset = 0;
        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 1, &doffset);

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(skybox);
}

void vk_engine::texture_init()
{
    /* to init a cs you need an allocator (custom struct) */
    comp_allocator allocator(_device, _allocator);

    allocator.create_img(VK_FORMAT_R16G16B16A16_SFLOAT, VkExtent3D{256, 256, 256},
                         VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_STORAGE_BIT, 0,
                         "cloud");

    /* match set binding */
    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloud"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
    };

    cs cloudtex(allocator, descriptors, "../shaders/cloudtex.comp.spv",
                _min_buffer_alignment);

    /* start building pipeline using info from struct cs and comp_allocator */
    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, cloudtex.module));

    VkPushConstantRange u_time = {};
    u_time.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time.offset = 0;
    u_time.size = sizeof(float);

    std::vector<VkDescriptorSetLayout> layouts = {
        cloudtex.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {
        u_time,
    };

    pb.build_comp(_device, layouts, push_constants, &cloudtex.pipeline_layout,
                  &cloudtex.pipeline);

    cloudtex.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vk_cmd::vk_img_layout_transition(cbuffer, cs->allocator.get_img("cloud").img,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_GENERAL, _comp_index);

        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        uint32_t doffset = 0;
        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 1, &doffset);

        vkCmdDispatch(cbuffer, 256 / 8, 256 / 8, 256 / 8);
    };

    cs::cc_init(_comp_index, _device);
    cs::comp_immediate_submit(_device, _comp_queue, &cloudtex);
}

void vk_engine::marching_init()
{
    comp_allocator allocator(_device, _allocator);

    struct camera_data {
        float znear;
        float zfar;
        glm::vec4 pos;
        glm::vec4 dir;
    };

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(camera_data)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "camera");

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloud"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
    };

    cs marching(allocator, descriptors, "../shaders/marching.comp.spv",
                _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, marching.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        marching.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &marching.pipeline_layout,
                  &marching.pipeline);

    marching.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        camera_data camera_data;
        camera_data.znear = 0.1f;
        camera_data.zfar = 100.f;
        camera_data.pos = glm::vec4(_vk_camera.get_pos(), 1.f);
        camera_data.dir = glm::vec4(_vk_camera.get_dir(), 1.f);

        void *data;
        vmaMapMemory(_allocator, cs->allocator.get_buffer("camera").allocation, &data);
        std::memcpy(data, &camera_data, pad_uniform_buffer_size(sizeof(camera_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("camera").allocation);

        std::vector<uint32_t> doffsets = {
            0,
            0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(marching);
}

void vk_engine::draw_comp(frame *frame)
{
    vk_cmd::vk_img_layout_transition(frame->cbuffer, _target.img,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_GENERAL, _comp_index);

    for (cs &cs : css)
        cs.draw(frame->cbuffer, &cs);

    vk_cmd::vk_img_layout_transition(frame->cbuffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     _comp_index);
}
